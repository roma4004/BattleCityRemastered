#pragma once

#include "enums/PlayerSlot.h"
#include <array>
#include <boost/asio/ip/udp.hpp>
#include <optional>
#include <span>

namespace network
{
using boost::asio::ip::udp;

//NOTE: who sat in each seat last, so a player coming back gets the seat left behind rather than the first free one
class SeatHistory final
{
public:
	void Seat(PlayerSlot slot, const udp::endpoint& endpoint);

	//NOTE: the seat left from this very address and port, then from this machine - a restarted game dials from
	//another port - then one nobody sat in, and only then one somebody else may still come back to
	[[nodiscard]] std::optional<PlayerSlot> Choose(std::span<const PlayerSlot> freeSeats,
												   const udp::endpoint& endpoint) const;

private:
	std::array<std::optional<udp::endpoint>, kSeatCount> _lastSeated{};
};
}//namespace network
