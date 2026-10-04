#include "network/SeatHistory.h"
#include "enums/PlayerSlot.h"
#include <algorithm>
#include <optional>
#include <span>

namespace network
{
void SeatHistory::Seat(const PlayerSlot slot, const udp::endpoint& endpoint)
{
	_lastSeated[SeatIndex(slot)] = endpoint;
}

std::optional<PlayerSlot> SeatHistory::Choose(const std::span<const PlayerSlot> freeSeats,
											  const udp::endpoint& endpoint) const
{
	const auto rank = [this, &endpoint](const PlayerSlot slot)
	{
		const std::optional<udp::endpoint>& last{_lastSeated[SeatIndex(slot)]};
		if (!last)
		{
			return 2;
		}

		if (*last == endpoint)
		{
			return 0;
		}

		return last->address() == endpoint.address() ? 1 : 3;
	};

	//NOTE: the first of the best - among equals, the seats still go out in order
	const auto best{std::ranges::min_element(freeSeats, {}, rank)};

	return best == freeSeats.end() ? std::nullopt : std::optional{*best};
}
}//namespace network
