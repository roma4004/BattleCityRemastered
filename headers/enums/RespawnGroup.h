#pragma once

#include "enums/PlayerSlot.h"
#include <array>
#include <cstddef>

enum class RespawnGroup : char8_t
{
	ENEMY_ALL,
	PLAYER1,
	PLAYER2,
	PLAYER3,
	PLAYER4
};

//NOTE: the enemy pool and one group per seat
inline constexpr std::size_t kRespawnGroupCount{1u + kSeatCount};

[[nodiscard]] constexpr RespawnGroup GroupOf(const PlayerSlot slot) noexcept
{
	constexpr std::array kGroups{RespawnGroup::PLAYER1, RespawnGroup::PLAYER2, RespawnGroup::PLAYER3,
								 RespawnGroup::PLAYER4};

	return kGroups[SeatIndex(slot)];
}
