#pragma once

#include "WireFrame.h"
#include "utils/Uuid.h"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace network
{
struct DatagramHeader final
{
	std::uint32_t connectionId{};
	//NOTE: the sender has not heard this peer yet - what tells a server a new client from a stale datagram
	bool isHello{};
};

//NOTE: one peer's side of the protocol, with no socket and no clock - the owner feeds datagrams in and puts
//what comes back on the wire, so every rule here runs in a test without a network
class DatagramLink final
{
public:
	using Clock = std::chrono::steady_clock;

	struct Arrivals final
	{
		//NOTE: whole, in the order they were sent, each exactly once
		std::vector<std::string> messages{};
		//NOTE: each newer than what this link handed over for its entity, none older than the last snapshot
		std::vector<std::string> latest{};
	};

	DatagramLink(std::uint32_t connectionId, Clock::time_point now);

	[[nodiscard]] static std::optional<DatagramHeader> PeekHeader(std::string_view datagram);

	//NOTE: false when the backlog would outgrow kMaxBacklog - it goes whole and the peer skips past the gap
	[[nodiscard]] bool SendReliable(std::string_view message);

	//NOTE: a reliable message replacing the peer's whole field - it opens an epoch, and a Latest value of an
	//older one is dropped on both ends
	[[nodiscard]] bool SendSnapshot(std::string_view message);

	//NOTE: false when the value alone would not fit a datagram - nothing here splits one
	bool SendLatest(const Uuid& key, std::string message);

	[[nodiscard]] bool SendFrame(const WireFrame& frame);

	//NOTE: nullopt for a datagram that is not this link's or does not parse - the link is left as it was
	[[nodiscard]] std::optional<Arrivals> Receive(std::string_view datagram, Clock::time_point now);

	//NOTE: whatever is due by now - new data, retransmits, an owed ack or a heartbeat
	[[nodiscard]] std::vector<std::string> TakeDatagrams(Clock::time_point now);

	[[nodiscard]] bool IsDrained() const { return _unacked.empty(); }
	[[nodiscard]] bool IsAwaitingSnapshot() const { return !_heldLatest.empty(); }
	[[nodiscard]] bool IsSilent(const Clock::time_point now) const { return now - _lastReceivedAt > kPeerTimeout; }
	[[nodiscard]] bool HasHeardPeer() const { return _hasHeardPeer; }
	[[nodiscard]] std::uint32_t ConnectionId() const { return _connectionId; }
	[[nodiscard]] Clock::duration RetransmitTimeout() const;

	static constexpr std::size_t kMaxDatagramSize{1200u};
	static constexpr std::size_t kMaxBacklog{1024u};
	static constexpr std::chrono::milliseconds kHeartbeat{100};
	static constexpr std::chrono::milliseconds kPeerTimeout{std::chrono::seconds{2}};
	//NOTE: how often the owner hands out what is due - retransmits and heartbeats keep no timer of their own
	static constexpr std::chrono::milliseconds kPollInterval{10};
	//NOTE: how long a goodbye is retransmitted before the owner closes without its ack
	static constexpr std::chrono::milliseconds kFarewellLinger{250};
	//NOTE: a snapshot leaves as a burst of up to a send window of datagrams - the OS default drops most of it
	static constexpr int kSocketBufferSize{1 << 20};

private:
	struct Fragment final
	{
		std::string bytes{};
		bool isLast{};
		bool isSnapshot{};
		std::uint32_t epoch{};
		unsigned sends{};
		Clock::time_point resendAt{};
	};

	struct LatestValue final
	{
		std::string bytes{};
		std::uint32_t version{};
		std::uint32_t epoch{};
		Clock::time_point resendAt{};
	};

	//NOTE: a value that came ahead of the snapshot opening its epoch
	struct HeldLatest final
	{
		std::string bytes{};
		std::uint32_t epoch{};
		std::uint32_t seq{};
	};

	//NOTE: what a datagram carried, so an ack for it can be credited to the data inside
	struct SentDatagram final
	{
		std::uint32_t seq{};
		Clock::time_point sentAt{};
		std::vector<std::uint32_t> fragments{};
		std::vector<std::pair<Uuid, std::uint32_t>> latest{};
	};

	[[nodiscard]] bool Enqueue(std::string_view message, bool isSnapshot);
	[[nodiscard]] std::uint32_t ReliableBase() const;
	void Acknowledge(std::uint32_t ack, std::uint32_t ackBits, Clock::time_point now);
	void Credit(const SentDatagram& sent, Clock::time_point now);
	void SampleRoundTrip(Clock::duration sample);
	void RecordArrival(std::uint32_t seq);
	void SkipTo(std::uint32_t base);
	void DeliverInOrder(Arrivals& arrivals);
	void EnterEpoch(std::uint32_t epoch, Arrivals& arrivals);
	void AcceptLatest(const Uuid& key, const HeldLatest& value, Arrivals& arrivals);

	const std::uint32_t _connectionId;
	bool _hasHeardPeer{};
	bool _isAckOwed{};
	Clock::time_point _lastReceivedAt;
	Clock::time_point _lastSentAt{};

	std::uint32_t _nextDatagramSeq{1u};
	std::uint32_t _nextReliableSeq{};
	std::map<std::uint32_t, Fragment> _unacked{};
	std::unordered_map<Uuid, LatestValue> _latest{};
	std::uint32_t _latestVersion{};
	//NOTE: how many snapshots this end has sent
	std::uint32_t _epoch{};
	std::deque<SentDatagram> _sent{};
	std::uint32_t _newestAck{};

	std::optional<Clock::duration> _smoothedRoundTrip{};
	Clock::duration _roundTripVariance{};

	//NOTE: 0 until the peer's first datagram - its numbering starts at 1
	std::uint32_t _newestReceived{};
	std::uint32_t _receivedBits{};
	std::uint32_t _nextDelivered{};
	std::map<std::uint32_t, Fragment> _outOfOrder{};
	std::string _assembly{};
	std::unordered_map<Uuid, std::uint32_t> _latestSeenIn{};
	std::uint32_t _appliedEpoch{};
	std::unordered_map<Uuid, HeldLatest> _heldLatest{};
};
}//namespace network
