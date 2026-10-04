#include "enums/PlayerSlot.h"
#include "network/SeatHistory.h"
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/udp.hpp>
#include <array>
#include <optional>
#include "gtest/gtest.h"

using boost::asio::ip::make_address;
using boost::asio::ip::udp;

// the seat a client dialling in is given - its own from before over a free one, and nobody else's while it can
class SeatHistoryTest : public testing::Test
{
protected:
	network::SeatHistory _history{};
};

// two windows on one machine - the address alone cannot tell them apart, the port can
TEST_F(SeatHistoryTest, TheSeatLeftFromThisVeryPortComesFirst)
{
	_history.Seat(PlayerSlot::P1, udp::endpoint{make_address("10.0.0.1"), 5000u});
	_history.Seat(PlayerSlot::P2, udp::endpoint{make_address("10.0.0.1"), 6000u});
	constexpr std::array freeSeats{PlayerSlot::P1, PlayerSlot::P2};

	EXPECT_EQ(_history.Choose(freeSeats, udp::endpoint{make_address("10.0.0.1"), 6000u}), PlayerSlot::P2);
}

// a restarted game dials from another port, and the machine still finds its seat
TEST_F(SeatHistoryTest, AGameRestartedOnTheSameMachineGetsItsSeatBack)
{
	_history.Seat(PlayerSlot::P1, udp::endpoint{make_address("10.0.0.1"), 5000u});
	_history.Seat(PlayerSlot::P2, udp::endpoint{make_address("10.0.0.2"), 5000u});
	constexpr std::array freeSeats{PlayerSlot::P1, PlayerSlot::P2};

	EXPECT_EQ(_history.Choose(freeSeats, udp::endpoint{make_address("10.0.0.2"), 7000u}), PlayerSlot::P2);
}

// a newcomer leaves the seat of one who left alone while there is one nobody sat in
TEST_F(SeatHistoryTest, ANewcomerTakesASeatNobodySatInOverOneSomebodyLeft)
{
	_history.Seat(PlayerSlot::P1, udp::endpoint{make_address("10.0.0.1"), 5000u});
	_history.Seat(PlayerSlot::P2, udp::endpoint{make_address("10.0.0.2"), 5000u});
	constexpr std::array freeSeats{PlayerSlot::P2, PlayerSlot::P3};

	EXPECT_EQ(_history.Choose(freeSeats, udp::endpoint{make_address("10.0.0.9"), 5000u}), PlayerSlot::P3);
}

// with only seats somebody left, the newcomer takes the first of them
TEST_F(SeatHistoryTest, WithOnlyLeftSeatsTheFirstGoes)
{
	_history.Seat(PlayerSlot::P1, udp::endpoint{make_address("10.0.0.1"), 5000u});
	_history.Seat(PlayerSlot::P2, udp::endpoint{make_address("10.0.0.2"), 5000u});
	constexpr std::array freeSeats{PlayerSlot::P1, PlayerSlot::P2};

	EXPECT_EQ(_history.Choose(freeSeats, udp::endpoint{make_address("10.0.0.9"), 5000u}), PlayerSlot::P1);
}

// a full match has no seat to give, whoever sat in it before
TEST_F(SeatHistoryTest, NoFreeSeatIsNoSeat)
{
	_history.Seat(PlayerSlot::P1, udp::endpoint{make_address("10.0.0.1"), 5000u});

	EXPECT_EQ(_history.Choose({}, udp::endpoint{make_address("10.0.0.1"), 5000u}), std::nullopt);
}
