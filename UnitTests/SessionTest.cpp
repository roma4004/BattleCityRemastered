#include "components/EventSystem.h"
#include "enums/PlayerSlot.h"
#include "network/DatagramLink.h"
#include "network/Session.h"
#include "network/WireFrame.h"
#include "gtest/gtest.h"
#include <boost/asio/ip/udp.hpp>
#include <cstddef>
#include <memory>

// A client that acks nothing is not sent the tail it missed - its session drops the backlog and owes a
// snapshot, paid in place of the next frame
class SessionTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{std::make_shared<EventSystem>()};
	std::shared_ptr<network::commands::Session> _session{std::make_shared<network::commands::Session>(
			boost::asio::ip::udp::endpoint{}, 7u, _events, PlayerSlot::P2, network::DatagramLink::Clock::now())};
	network::WireFrame _frame{.reliable = "frame", .latest = {}, .isSnapshot = false};
};

// a client whose backlog was given up on has missed part of the field, so it is owed the whole of it
TEST_F(SessionTest, AClientThatStopsAckingIsOwedASnapshotOnceItsBacklogIsDropped)
{
	for (std::size_t i = 0u; i < network::DatagramLink::kMaxBacklog; ++i)
	{
		_session->Send(_frame);
	}
	ASSERT_FALSE(_session->IsSnapshotOwed()) << "a full backlog is not yet an overflow";

	_session->Send(_frame);

	EXPECT_TRUE(_session->IsSnapshotOwed());
}
