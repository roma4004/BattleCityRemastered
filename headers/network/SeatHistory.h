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

	//NOTE: this endpoint's seat, then this machine's, then a never-used one, then one a leaver may still come back to
	[[nodiscard]] std::optional<PlayerSlot> Choose(std::span<const PlayerSlot> freeSeats,
												   const udp::endpoint& endpoint) const;

private:
	std::array<std::optional<udp::endpoint>, kSeatCount> _lastSeated{};
};
}//namespace network
