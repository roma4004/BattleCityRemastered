#include "network/DatagramLink.h"
#include "utils/Uuid.h"
#include "utils/UuidUtils.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

using namespace std::chrono_literals;

// Both ends of one link, and a wire that is nothing but this test - it drops, repeats and reorders by hand
class DatagramLinkTest : public testing::Test
{
protected:
	using Clock = network::DatagramLink::Clock;

	Clock::time_point _now{Clock::time_point{} + 1h};
	network::DatagramLink _host{7u, _now};
	network::DatagramLink _client{7u, _now};
	Uuid _tank{UuidUtils::GetRandomUuid()};

	std::vector<std::string> _messages{};
	std::vector<std::string> _latest{};

	void Carry(const std::vector<std::string>& datagrams, network::DatagramLink& to)
	{
		for (const std::string& datagram: datagrams)
		{
			if (auto arrivals{to.Receive(datagram, _now)})
			{
				std::ranges::move(arrivals->messages, std::back_inserter(_messages));
				std::ranges::move(arrivals->latest, std::back_inserter(_latest));
			}
		}
	}
};

TEST_F(DatagramLinkTest, AMessageArrivesWhole)
{
	ASSERT_TRUE(_host.SendReliable("hello"));

	Carry(_host.TakeDatagrams(_now), _client);

	EXPECT_EQ(_messages, std::vector<std::string>{"hello"});
}

TEST_F(DatagramLinkTest, ADuplicatedDatagramDeliversOnce)
{
	ASSERT_TRUE(_host.SendReliable("hello"));
	const auto datagrams{_host.TakeDatagrams(_now)};

	Carry(datagrams, _client);
	Carry(datagrams, _client);

	EXPECT_EQ(_messages, std::vector<std::string>{"hello"});
}

TEST_F(DatagramLinkTest, ALostDatagramIsSentAgainOnlyOnceTheRetransmitTimeoutPasses)
{
	ASSERT_TRUE(_host.SendReliable("hello"));
	std::ignore = _host.TakeDatagrams(_now);

	_now += 10ms;
	Carry(_host.TakeDatagrams(_now), _client);
	ASSERT_TRUE(_messages.empty()) << "resent before its timeout";

	_now += _host.RetransmitTimeout();
	Carry(_host.TakeDatagrams(_now), _client);

	EXPECT_EQ(_messages, std::vector<std::string>{"hello"});
}

TEST_F(DatagramLinkTest, AnAcknowledgedMessageIsNotSentAgain)
{
	ASSERT_TRUE(_host.SendReliable("hello"));
	Carry(_host.TakeDatagrams(_now), _client);
	Carry(_client.TakeDatagrams(_now), _host);

	_now += 1s;
	Carry(_host.TakeDatagrams(_now), _client);

	EXPECT_EQ(_messages, std::vector<std::string>{"hello"});
	EXPECT_TRUE(_host.IsDrained());
}

TEST_F(DatagramLinkTest, ReorderedDatagramsDeliverInTheOrderSent)
{
	ASSERT_TRUE(_host.SendReliable("first"));
	const auto first{_host.TakeDatagrams(_now)};
	ASSERT_TRUE(_host.SendReliable("second"));
	const auto second{_host.TakeDatagrams(_now)};

	Carry(second, _client);
	ASSERT_TRUE(_messages.empty()) << "the second message overtook the first";
	Carry(first, _client);

	EXPECT_EQ(_messages, (std::vector<std::string>{"first", "second"}));
}

TEST_F(DatagramLinkTest, AMessageLargerThanADatagramIsSplitAndPutBackTogether)
{
	std::string snapshot(5000u, '\0');
	std::ranges::generate(snapshot, [i = 0]() mutable { return static_cast<char>(i++ % 251); });
	ASSERT_TRUE(_host.SendReliable(snapshot));

	auto datagrams{_host.TakeDatagrams(_now)};
	ASSERT_GE(datagrams.size(), 5u);
	EXPECT_TRUE(std::ranges::all_of(datagrams, [](const std::string& datagram)
	{
		return datagram.size() <= network::DatagramLink::kMaxDatagramSize;
	}));

	std::ranges::reverse(datagrams);
	Carry(datagrams, _client);

	ASSERT_EQ(_messages.size(), 1u);
	EXPECT_EQ(_messages.front(), snapshot);
}

TEST_F(DatagramLinkTest, OnlyTheNewestValueOfAnEntityGoesOut)
{
	_host.SendLatest(_tank, "old");
	_host.SendLatest(_tank, "new");

	Carry(_host.TakeDatagrams(_now), _client);

	EXPECT_EQ(_latest, std::vector<std::string>{"new"});
}

TEST_F(DatagramLinkTest, AnOutdatedValueArrivingLateIsDropped)
{
	_host.SendLatest(_tank, "old");
	const auto old{_host.TakeDatagrams(_now)};
	_host.SendLatest(_tank, "new");
	const auto fresh{_host.TakeDatagrams(_now)};

	Carry(fresh, _client);
	Carry(old, _client);

	EXPECT_EQ(_latest, std::vector<std::string>{"new"});
}

//NOTE: a tank that stopped sends no further position, so the last one lost would leave it standing elsewhere
TEST_F(DatagramLinkTest, ALostValueIsSentAgainWhileItIsStillTheNewest)
{
	_host.SendLatest(_tank, "stopped");
	std::ignore = _host.TakeDatagrams(_now);

	_now += _host.RetransmitTimeout();
	Carry(_host.TakeDatagrams(_now), _client);

	EXPECT_EQ(_latest, std::vector<std::string>{"stopped"});
}

TEST_F(DatagramLinkTest, ALostValueAlreadyReplacedIsNeverSentAgain)
{
	_host.SendLatest(_tank, "old");
	std::ignore = _host.TakeDatagrams(_now);
	_host.SendLatest(_tank, "new");
	Carry(_host.TakeDatagrams(_now), _client);
	Carry(_client.TakeDatagrams(_now), _host);

	_now += 1s;
	Carry(_host.TakeDatagrams(_now), _client);

	EXPECT_EQ(_latest, std::vector<std::string>{"new"});
}

//NOTE: a settled entity is forgotten, so the ack for a late copy of its old value may arrive after a new one
TEST_F(DatagramLinkTest, ALateAckForAnOlderValueDoesNotSettleTheNewestOne)
{
	_host.SendLatest(_tank, "old");
	const auto first{_host.TakeDatagrams(_now)};
	_now += _host.RetransmitTimeout();
	const auto lateCopy{_host.TakeDatagrams(_now)};
	Carry(first, _client);
	Carry(_client.TakeDatagrams(_now), _host);

	_host.SendLatest(_tank, "new");
	std::ignore = _host.TakeDatagrams(_now);
	Carry(lateCopy, _client);
	Carry(_client.TakeDatagrams(_now), _host);
	_latest.clear();

	_now += _host.RetransmitTimeout();
	Carry(_host.TakeDatagrams(_now), _client);

	EXPECT_EQ(_latest, std::vector<std::string>{"new"});
}

//NOTE: a tank the snapshot put in place would be moved back to where it stood before the snapshot was taken
TEST_F(DatagramLinkTest, AValueSentBeforeASnapshotAndArrivingAfterItIsDropped)
{
	_host.SendLatest(_tank, "before");
	const auto late{_host.TakeDatagrams(_now)};
	ASSERT_TRUE(_host.SendSnapshot("snapshot"));
	Carry(_host.TakeDatagrams(_now), _client);

	Carry(late, _client);

	EXPECT_EQ(_messages, std::vector<std::string>{"snapshot"});
	EXPECT_TRUE(_latest.empty());
}

//NOTE: applied at once, the value would be wiped by the snapshot still on its way
TEST_F(DatagramLinkTest, AValueArrivingAheadOfItsSnapshotIsHandedOverRightAfterIt)
{
	ASSERT_TRUE(_host.SendSnapshot("snapshot"));
	const auto snapshot{_host.TakeDatagrams(_now)};
	_host.SendLatest(_tank, "after");
	Carry(_host.TakeDatagrams(_now), _client);
	ASSERT_TRUE(_latest.empty());
	ASSERT_TRUE(_client.IsAwaitingSnapshot());

	Carry(snapshot, _client);

	EXPECT_EQ(_messages, std::vector<std::string>{"snapshot"});
	EXPECT_EQ(_latest, std::vector<std::string>{"after"});
	EXPECT_FALSE(_client.IsAwaitingSnapshot());
}

TEST_F(DatagramLinkTest, ASnapshotRetiresTheValuesItAlreadyHolds)
{
	_host.SendLatest(_tank, "unacked");
	std::ignore = _host.TakeDatagrams(_now);
	ASSERT_TRUE(_host.SendSnapshot("snapshot"));
	Carry(_host.TakeDatagrams(_now), _client);
	Carry(_client.TakeDatagrams(_now), _host);

	//NOTE: a peer that has seen nothing takes whatever still goes out
	_now += 1s;
	network::DatagramLink probe{7u, _now};
	Carry(_host.TakeDatagrams(_now), probe);

	EXPECT_TRUE(_latest.empty()) << "a value the snapshot already holds is still being resent";
}

TEST_F(DatagramLinkTest, AValueWaitingForASnapshotThatNeverCameGoesWithTheNextOne)
{
	ASSERT_TRUE(_host.SendSnapshot("snapshot that is dropped"));
	std::ignore = _host.TakeDatagrams(_now);
	_host.SendLatest(_tank, "waiting");
	Carry(_host.TakeDatagrams(_now), _client);
	ASSERT_TRUE(_client.IsAwaitingSnapshot());

	//NOTE: the backlog overflows and takes the first snapshot with it
	for (std::size_t i = 0u; i < network::DatagramLink::kMaxBacklog; ++i)
	{
		std::ignore = _host.SendReliable("frame");
	}
	ASSERT_TRUE(_host.SendSnapshot("next snapshot"));

	_now += 10ms;
	Carry(_host.TakeDatagrams(_now), _client);

	EXPECT_EQ(_messages, std::vector<std::string>{"next snapshot"});
	EXPECT_TRUE(_latest.empty());
	EXPECT_FALSE(_client.IsAwaitingSnapshot());
}

TEST_F(DatagramLinkTest, TheRetransmitTimeoutFollowsTheMeasuredRoundTrip)
{
	ASSERT_EQ(_host.RetransmitTimeout(), 200ms);

	ASSERT_TRUE(_host.SendReliable("hello"));
	const auto datagrams{_host.TakeDatagrams(_now)};
	_now += 20ms;
	Carry(datagrams, _client);
	Carry(_client.TakeDatagrams(_now), _host);

	//NOTE: RFC 6298 on a first sample - the round trip plus four times half of it
	EXPECT_EQ(_host.RetransmitTimeout(), 60ms);
}

// A peer that stopped acking is not sent the tail it missed - it is dropped, and the snapshot replacing it
// is what arrives first
TEST_F(DatagramLinkTest, ABacklogPastTheLimitIsDroppedAndThePeerSkipsToTheNextMessage)
{
	for (std::size_t i = 0u; i < network::DatagramLink::kMaxBacklog; ++i)
	{
		ASSERT_TRUE(_host.SendReliable("frame " + std::to_string(i)));
	}
	std::ignore = _host.TakeDatagrams(_now);

	EXPECT_FALSE(_host.SendReliable("one frame too many"));
	ASSERT_TRUE(_host.SendReliable("snapshot"));

	_now += 10ms;
	Carry(_host.TakeDatagrams(_now), _client);

	EXPECT_EQ(_messages, std::vector<std::string>{"snapshot"});
}

TEST_F(DatagramLinkTest, AQuietPeerKeptAliveByHeartbeatsIsNotSilent)
{
	for (auto elapsed{0ms}; elapsed < 3s; elapsed += 100ms)
	{
		_now += 100ms;
		Carry(_host.TakeDatagrams(_now), _client);
	}

	EXPECT_FALSE(_client.IsSilent(_now));

	_now += network::DatagramLink::kPeerTimeout + 1ms;
	EXPECT_TRUE(_client.IsSilent(_now));
}

TEST_F(DatagramLinkTest, ADatagramOfAnotherConnectionIsIgnored)
{
	network::DatagramLink stranger{8u, _now};
	ASSERT_TRUE(stranger.SendReliable("not yours"));

	for (const std::string& datagram: stranger.TakeDatagrams(_now))
	{
		EXPECT_FALSE(_client.Receive(datagram, _now).has_value());
	}
}
