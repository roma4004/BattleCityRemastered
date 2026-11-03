#pragma once

#include "enums/PlayerSlot.h"
#include <array>
#include <boost/asio/ip/udp.hpp>
#include <cstdint>
#include <optional>
#include <span>

namespace network
{
using boost::asio::ip::udp;

//NOTE: who sat in each seat last, so a player coming back gets the seat left behind rather than the first free one
class SeatHistory final
{
public:
	void Seat(PlayerSlot slot, const udp::endpoint& endpoint, std::uint64_t playerKey);

	//NOTE: this endpoint's seat, then this player's, then this machine's, then a never-used one, then anybody's
	[[nodiscard]] std::optional<PlayerSlot> Choose(std::span<const PlayerSlot> freeSeats, const udp::endpoint& endpoint,
												   std::uint64_t playerKey) const;

	//NOTE: the free seat this player left - none for a newcomer, even behind the same NAT
	[[nodiscard]] std::optional<PlayerSlot> Returning(std::span<const PlayerSlot> freeSeats,
													  const udp::endpoint& endpoint, std::uint64_t playerKey) const;

private:
	struct Holder final
	{
		udp::endpoint endpoint{};
		std::uint64_t playerKey{};
	};

	std::array<std::optional<Holder>, kSeatCount> _lastSeated{};
};
}//namespace network
