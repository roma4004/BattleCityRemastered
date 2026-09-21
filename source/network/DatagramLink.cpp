#include "network/DatagramLink.h"
#include "network/WireFrame.h"
#include "utils/Uuid.h"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

namespace network
{
namespace
{
constexpr std::uint16_t kProtocolId{0xBC54u};
constexpr std::uint8_t kHelloFlag{0x01u};
constexpr std::uint8_t kLastFragmentFlag{0x01u};
constexpr std::uint8_t kSnapshotFragmentFlag{0x02u};

enum class ChunkType : std::uint8_t
{
	Reliable = 1,
	Latest = 2,
};

//NOTE: protocol id, flags, connection id, datagram seq, ack, ack bits, reliable base
constexpr std::size_t kHeaderSize{2u + 1u + 4u * 5u};
constexpr std::size_t kReliableChunkHeaderSize{1u + 1u + 4u + 4u + 2u};
constexpr std::size_t kMaxFragmentSize{DatagramLink::kMaxDatagramSize - kHeaderSize - kReliableChunkHeaderSize};
//NOTE: chunk type, entity, epoch, size
constexpr std::size_t kLatestChunkHeaderSize{1u + sizeof(Uuid) + 4u + 2u};
//NOTE: nothing splits a Latest value - it is one entity's newest state, and half of it is worth nothing
constexpr std::size_t kMaxLatestSize{DatagramLink::kMaxDatagramSize - kHeaderSize - kLatestChunkHeaderSize};

constexpr std::uint32_t kAckBits{32u};
//NOTE: kept below the receive window, so a fragment the sender may send always fits where the peer stores it
constexpr std::uint32_t kSendWindow{256u};
constexpr std::uint32_t kReceiveWindow{1024u};
//NOTE: how many datagrams back an entity's newest value is still remembered
constexpr std::uint32_t kLatestMemory{4096u};
constexpr std::size_t kMaxSentRecords{1024u};

constexpr auto kInitialRetransmit{200ms};
constexpr auto kMinRetransmit{50ms};
constexpr auto kMaxRetransmit{1s};

void PutU8(std::string& out, const std::uint8_t value) { out.push_back(static_cast<char>(value)); }

void PutU16(std::string& out, const std::uint16_t value)
{
	PutU8(out, static_cast<std::uint8_t>(value >> 8u));
	PutU8(out, static_cast<std::uint8_t>(value & 0xFFu));
}

void PutU32(std::string& out, const std::uint32_t value)
{
	PutU16(out, static_cast<std::uint16_t>(value >> 16u));
	PutU16(out, static_cast<std::uint16_t>(value & 0xFFFFu));
}

//NOTE: every read is bounds-checked once - a short datagram flips ok instead of reading past its end
struct Reader final
{
	std::string_view data;
	std::size_t offset{};
	bool ok{true};

	[[nodiscard]] std::string_view Take(const std::size_t size)
	{
		if (!ok || data.size() - offset < size)
		{
			ok = false;
			return {};
		}

		offset += size;

		return data.substr(offset - size, size);
	}

	[[nodiscard]] std::uint32_t TakeUnsigned(const std::size_t size)
	{
		std::uint32_t value{};
		for (const char byte: Take(size))
		{
			value = (value << 8u) | static_cast<unsigned char>(byte);
		}

		return value;
	}

	[[nodiscard]] bool IsAtEnd() const { return offset == data.size(); }
};

struct ParsedHeader final
{
	DatagramHeader header{};
	std::uint32_t seq{};
	std::uint32_t ack{};
	std::uint32_t ackBits{};
	std::uint32_t reliableBase{};
};

[[nodiscard]] std::optional<ParsedHeader> ParseHeader(Reader& reader)
{
	const auto protocolId{reader.TakeUnsigned(2u)};
	const auto flags{reader.TakeUnsigned(1u)};
	ParsedHeader parsed{.header = {.connectionId = reader.TakeUnsigned(4u), .isHello = (flags & kHelloFlag) != 0u},
						.seq = reader.TakeUnsigned(4u),
						.ack = reader.TakeUnsigned(4u),
						.ackBits = reader.TakeUnsigned(4u),
						.reliableBase = reader.TakeUnsigned(4u)};

	if (!reader.ok || protocolId != kProtocolId || parsed.seq == 0u)
	{
		return std::nullopt;
	}

	return parsed;
}

struct ReliableChunk final
{
	std::uint32_t seq{};
	bool isLast{};
	bool isSnapshot{};
	std::uint32_t epoch{};
	std::string_view bytes{};
};

struct LatestChunk final
{
	Uuid key{};
	std::uint32_t epoch{};
	std::string_view bytes{};
};
}//namespace

DatagramLink::DatagramLink(const std::uint32_t connectionId, const Clock::time_point now)
	: _connectionId{connectionId}
	, _lastReceivedAt{now} {}

std::optional<DatagramHeader> DatagramLink::PeekHeader(const std::string_view datagram)
{
	Reader reader{.data = datagram};
	const auto parsed{ParseHeader(reader)};

	return parsed ? std::optional{parsed->header} : std::nullopt;
}

DatagramLink::Clock::duration DatagramLink::RetransmitTimeout() const
{
	if (!_smoothedRoundTrip)
	{
		return kInitialRetransmit;
	}

	const Clock::duration estimate{*_smoothedRoundTrip + 4 * _roundTripVariance};

	return std::clamp<Clock::duration>(estimate, kMinRetransmit, kMaxRetransmit);
}

std::uint32_t DatagramLink::ReliableBase() const
{
	return _unacked.empty() ? _nextReliableSeq : _unacked.begin()->first;
}

bool DatagramLink::SendReliable(const std::string_view message) { return Enqueue(message, false); }

bool DatagramLink::SendSnapshot(const std::string_view message)
{
	++_epoch;
	//NOTE: the snapshot holds these values already - resent, they would be dropped as the epoch before it
	_latest.clear();

	return Enqueue(message, true);
}

bool DatagramLink::Enqueue(const std::string_view message, const bool isSnapshot)
{
	const std::size_t fragmentCount{(message.size() + kMaxFragmentSize - 1u) / kMaxFragmentSize};
	if (_unacked.size() + fragmentCount > kMaxBacklog)
	{
		_unacked.clear();

		return false;
	}

	for (std::size_t offset = 0u; offset < message.size(); offset += kMaxFragmentSize)
	{
		const std::string_view piece{message.substr(offset, kMaxFragmentSize)};
		_unacked.emplace(_nextReliableSeq++, Fragment{.bytes = std::string{piece},
													  .isLast = offset + kMaxFragmentSize >= message.size(),
													  .isSnapshot = isSnapshot,
													  .epoch = _epoch});
	}

	return true;
}

bool DatagramLink::SendLatest(const Uuid& key, std::string message)
{
	if (message.size() > kMaxLatestSize)
	{
		return false;
	}

	//NOTE: numbered across the link - an entry acked and erased comes back, and an old ack must not match it
	_latest.insert_or_assign(key, LatestValue{.bytes = std::move(message),
											  .version = ++_latestVersion,
											  .epoch = _epoch,
											  .resendAt = {}});

	return true;
}

bool DatagramLink::SendFrame(const WireFrame& frame)
{
	const bool isQueued{frame.isSnapshot ? SendSnapshot(frame.reliable) : SendReliable(frame.reliable)};
	const auto isHeld = [this](const auto& latest) { return SendLatest(latest.first, latest.second); };

	//NOTE: counted, not short-circuited - one value too big must not keep the rest off the wire
	return std::ranges::count_if(frame.latest, isHeld) == std::ssize(frame.latest) && isQueued;
}

std::optional<DatagramLink::Arrivals> DatagramLink::Receive(const std::string_view datagram,
															const Clock::time_point now)
{
	Reader reader{.data = datagram};
	const auto parsed{ParseHeader(reader)};
	if (!parsed || parsed->header.connectionId != _connectionId)
	{
		return std::nullopt;
	}

	const std::uint32_t nextDelivered{std::max(_nextDelivered, parsed->reliableBase)};
	std::vector<ReliableChunk> reliable{};
	std::vector<LatestChunk> latest{};
	while (reader.ok && !reader.IsAtEnd())
	{
		const auto type{static_cast<ChunkType>(reader.TakeUnsigned(1u))};
		if (type == ChunkType::Reliable)
		{
			const auto flags{reader.TakeUnsigned(1u)};
			const ReliableChunk chunk{.seq = reader.TakeUnsigned(4u),
									  .isLast = (flags & kLastFragmentFlag) != 0u,
									  .isSnapshot = (flags & kSnapshotFragmentFlag) != 0u,
									  .epoch = reader.TakeUnsigned(4u),
									  .bytes = reader.Take(reader.TakeUnsigned(2u))};
			//NOTE: past the window nothing honest could have sent it - storing it would let the wire grow the buffer
			reader.ok = reader.ok && (chunk.seq < nextDelivered || chunk.seq - nextDelivered < kReceiveWindow);
			reliable.push_back(chunk);
		}
		else if (type == ChunkType::Latest)
		{
			LatestChunk chunk{};
			std::ranges::transform(reader.Take(sizeof(Uuid)), std::begin(chunk.key.data),
								   [](const char byte) { return static_cast<std::uint8_t>(byte); });
			chunk.epoch = reader.TakeUnsigned(4u);
			chunk.bytes = reader.Take(reader.TakeUnsigned(2u));
			latest.push_back(chunk);
		}
		else
		{
			reader.ok = false;
		}
	}

	if (!reader.ok)
	{
		return std::nullopt;
	}

	_lastReceivedAt = now;
	_hasHeardPeer = true;
	Acknowledge(parsed->ack, parsed->ackBits, now);

	RecordArrival(parsed->seq);
	_isAckOwed = _isAckOwed || !reliable.empty() || !latest.empty();
	SkipTo(parsed->reliableBase);

	for (const auto& [seq, isLast, isSnapshot, epoch, bytes]: reliable)
	{
		if (seq >= _nextDelivered)
		{
			_outOfOrder.try_emplace(seq, Fragment{.bytes = std::string{bytes},
												  .isLast = isLast,
												  .isSnapshot = isSnapshot,
												  .epoch = epoch});
		}
	}

	Arrivals arrivals{};
	DeliverInOrder(arrivals);

	if (parsed->seq + kLatestMemory < _newestReceived)
	{
		return arrivals;
	}

	for (const auto& [key, epoch, bytes]: latest)
	{
		AcceptLatest(key, HeldLatest{.bytes = std::string{bytes}, .epoch = epoch, .seq = parsed->seq}, arrivals);
	}

	std::erase_if(_latestSeenIn, [this](const auto& seen) { return seen.second + kLatestMemory < _newestReceived; });
	std::erase_if(_heldLatest, [this](const auto& held) { return held.second.seq + kLatestMemory < _newestReceived; });

	return arrivals;
}

//NOTE: a value belongs to the snapshot it was sent after - an older one would move what that snapshot put
//in place, a newer one would be wiped by the snapshot still on its way
void DatagramLink::AcceptLatest(const Uuid& key, const HeldLatest& value, Arrivals& arrivals)
{
	if (value.epoch < _appliedEpoch)
	{
		return;
	}

	if (value.epoch > _appliedEpoch)
	{
		const auto [it, isNew]{_heldLatest.try_emplace(key, value)};
		if (!isNew && std::tie(it->second.epoch, it->second.seq) < std::tie(value.epoch, value.seq))
		{
			it->second = value;
		}

		return;
	}

	const auto [it, isNew]{_latestSeenIn.try_emplace(key, value.seq)};
	if (!isNew && it->second >= value.seq)
	{
		return;
	}

	it->second = value.seq;
	arrivals.latest.push_back(value.bytes);
}

//NOTE: a snapshot that never came is replaced by the next one, so what waited for it goes along with it
void DatagramLink::EnterEpoch(const std::uint32_t epoch, Arrivals& arrivals)
{
	_appliedEpoch = epoch;

	const auto isOpened = [epoch](const auto& held) { return held.second.epoch == epoch; };
	for (const auto& [key, value]: _heldLatest | std::views::filter(isOpened))
	{
		AcceptLatest(key, value, arrivals);
	}

	std::erase_if(_heldLatest, [epoch](const auto& held) { return held.second.epoch <= epoch; });
}

//NOTE: only what the ack says - a repeated datagram is told apart by its reliable seq and its entity's newest one
void DatagramLink::RecordArrival(const std::uint32_t seq)
{
	if (seq > _newestReceived)
	{
		const std::uint32_t shift{seq - _newestReceived};
		const bool hadAny{_newestReceived != 0u};
		_receivedBits = shift >= kAckBits ? 0u : _receivedBits << shift;
		if (hadAny && shift <= kAckBits)
		{
			_receivedBits |= 1u << (shift - 1u);
		}

		_newestReceived = seq;

		return;
	}

	if (const std::uint32_t distance{_newestReceived - seq}; distance > 0u && distance <= kAckBits)
	{
		_receivedBits |= 1u << (distance - 1u);
	}
}

//NOTE: the base only passes what the sender dropped - everything below it arrived here or is gone for good
void DatagramLink::SkipTo(const std::uint32_t base)
{
	if (base <= _nextDelivered)
	{
		return;
	}

	_outOfOrder.erase(_outOfOrder.begin(), _outOfOrder.lower_bound(base));
	_assembly.clear();
	_nextDelivered = base;
}

void DatagramLink::DeliverInOrder(Arrivals& arrivals)
{
	for (auto it{_outOfOrder.find(_nextDelivered)}; it != _outOfOrder.end(); it = _outOfOrder.find(_nextDelivered))
	{
		_assembly += it->second.bytes;
		if (it->second.isLast)
		{
			arrivals.messages.push_back(std::move(_assembly));
			_assembly.clear();

			if (it->second.isSnapshot)
			{
				EnterEpoch(it->second.epoch, arrivals);
			}
		}

		_outOfOrder.erase(it);
		++_nextDelivered;
	}
}

void DatagramLink::Acknowledge(const std::uint32_t ack, const std::uint32_t ackBits, const Clock::time_point now)
{
	const auto isAcked = [ack, ackBits](const SentDatagram& sent)
	{
		if (sent.seq > ack)
		{
			return false;
		}

		const std::uint32_t distance{ack - sent.seq};

		return distance == 0u || (distance <= kAckBits && ((ackBits >> (distance - 1u)) & 1u) != 0u);
	};

	std::ranges::for_each(_sent | std::views::filter(isAcked), [this, now](const SentDatagram& sent)
	{
		Credit(sent, now);
	});
	std::erase_if(_sent, isAcked);

	_newestAck = std::max(_newestAck, ack);
	std::erase_if(_sent, [this](const SentDatagram& sent) { return sent.seq + kAckBits < _newestAck; });
}

void DatagramLink::Credit(const SentDatagram& sent, const Clock::time_point now)
{
	if (!sent.fragments.empty() || !sent.latest.empty())
	{
		SampleRoundTrip(now - sent.sentAt);
	}

	std::ranges::for_each(sent.fragments, [this](const std::uint32_t seq) { _unacked.erase(seq); });

	for (const auto& [key, version]: sent.latest)
	{
		if (const auto it{_latest.find(key)}; it != _latest.end() && it->second.version == version)
		{
			_latest.erase(it);
		}
	}
}

//NOTE: RFC 6298 - only datagrams that carried data are acked at once, so a heartbeat never skews the sample
void DatagramLink::SampleRoundTrip(const Clock::duration sample)
{
	if (!_smoothedRoundTrip)
	{
		_smoothedRoundTrip = sample;
		_roundTripVariance = sample / 2;

		return;
	}

	const Clock::duration error{sample > *_smoothedRoundTrip ? sample - *_smoothedRoundTrip
															  : *_smoothedRoundTrip - sample};
	_roundTripVariance += (error - _roundTripVariance) / 4;
	*_smoothedRoundTrip += (sample - *_smoothedRoundTrip) / 8;
}

std::vector<std::string> DatagramLink::TakeDatagrams(const Clock::time_point now)
{
	const Clock::duration retransmit{RetransmitTimeout()};
	std::vector<std::string> datagrams{};
	std::string payload{};
	SentDatagram record{};

	const auto finish = [this, now, &datagrams, &payload, &record]
	{
		std::string datagram{};
		PutU16(datagram, kProtocolId);
		PutU8(datagram, _hasHeardPeer ? std::uint8_t{0u} : kHelloFlag);
		PutU32(datagram, _connectionId);
		PutU32(datagram, _nextDatagramSeq);
		PutU32(datagram, _newestReceived);
		PutU32(datagram, _receivedBits);
		PutU32(datagram, ReliableBase());
		datagram += payload;
		datagrams.push_back(std::move(datagram));

		record.seq = _nextDatagramSeq++;
		record.sentAt = now;
		_sent.push_back(std::move(record));
		record = {};
		payload.clear();
	};

	const auto append = [&payload, &finish](const std::string& chunk)
	{
		if (!payload.empty() && kHeaderSize + payload.size() + chunk.size() > kMaxDatagramSize)
		{
			finish();
		}

		payload += chunk;
	};

	const std::uint32_t windowEnd{ReliableBase() + kSendWindow};
	for (auto& [seq, fragment]: _unacked)
	{
		if (seq >= windowEnd)
		{
			break;
		}

		if (now < fragment.resendAt)
		{
			continue;
		}

		std::string chunk{};
		PutU8(chunk, static_cast<std::uint8_t>(ChunkType::Reliable));
		PutU8(chunk, static_cast<std::uint8_t>((fragment.isLast ? kLastFragmentFlag : 0u)
											   | (fragment.isSnapshot ? kSnapshotFragmentFlag : 0u)));
		PutU32(chunk, seq);
		PutU32(chunk, fragment.epoch);
		PutU16(chunk, static_cast<std::uint16_t>(fragment.bytes.size()));
		chunk += fragment.bytes;
		append(chunk);
		record.fragments.push_back(seq);

		//NOTE: doubled on every loss of the same fragment, so a dead peer is not flooded while it times out
		const unsigned backoff{std::min(fragment.sends, 4u)};
		fragment.resendAt = now + std::min<Clock::duration>(retransmit * (1u << backoff), kMaxRetransmit);
		++fragment.sends;
	}

	for (auto& [key, value]: _latest)
	{
		if (now < value.resendAt)
		{
			continue;
		}

		std::string chunk{};
		PutU8(chunk, static_cast<std::uint8_t>(ChunkType::Latest));
		std::ranges::transform(key.data, std::back_inserter(chunk),
							   [](const std::uint8_t byte) { return static_cast<char>(byte); });
		PutU32(chunk, value.epoch);
		PutU16(chunk, static_cast<std::uint16_t>(value.bytes.size()));
		chunk += value.bytes;
		append(chunk);
		record.latest.emplace_back(key, value.version);
		value.resendAt = now + retransmit;
	}

	if (!payload.empty() || (datagrams.empty() && (_isAckOwed || now - _lastSentAt >= kHeartbeat)))
	{
		finish();
	}

	if (!datagrams.empty())
	{
		_isAckOwed = false;
		_lastSentAt = now;
	}

	while (_sent.size() > kMaxSentRecords)
	{
		_sent.pop_front();
	}

	return datagrams;
}
}//namespace network
