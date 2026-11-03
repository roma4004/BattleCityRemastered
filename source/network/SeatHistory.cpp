#include "network/SeatHistory.h"
#include "enums/PlayerSlot.h"
#include <algorithm>
#include <cstdint>
#include <optional>
#include <ranges>
#include <span>
#include <vector>

namespace network
{
void SeatHistory::Seat(const PlayerSlot slot, const udp::endpoint& endpoint, const std::uint64_t playerKey)
{
	_lastSeated[SeatIndex(slot)] = Holder{.endpoint = endpoint, .playerKey = playerKey};
}

std::optional<PlayerSlot> SeatHistory::Choose(const std::span<const PlayerSlot> freeSeats,
											  const udp::endpoint& endpoint, const std::uint64_t playerKey) const
{
	const auto rank = [this, &endpoint, playerKey](const PlayerSlot slot)
	{
		const std::optional<Holder>& last{_lastSeated[SeatIndex(slot)]};
		if (!last)
		{
			return 3;
		}

		if (last->endpoint == endpoint)
		{
			return 0;
		}

		if (last->playerKey == playerKey)
		{
			return 1;
		}

		return last->endpoint.address() == endpoint.address() ? 2 : 4;
	};

	//NOTE: the first of the best - among equals, the seats still go out in order
	const auto best{std::ranges::min_element(freeSeats, {}, rank)};

	return best == freeSeats.end() ? std::nullopt : std::optional{*best};
}

std::optional<PlayerSlot> SeatHistory::Returning(const std::span<const PlayerSlot> freeSeats,
												 const udp::endpoint& endpoint, const std::uint64_t playerKey) const
{
	const auto isLeftByIt = [this, playerKey](const PlayerSlot slot)
	{
		const std::optional<Holder>& last{_lastSeated[SeatIndex(slot)]};

		return last && last->playerKey == playerKey;
	};
	const auto left{freeSeats | std::views::filter(isLeftByIt) | std::ranges::to<std::vector>()};

	return left.empty() ? std::nullopt : Choose(left, endpoint, playerKey);
}
}//namespace network
