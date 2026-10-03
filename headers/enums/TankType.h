#pragma once

#include "enums/PlayerSlot.h"
#include <array>
#include <optional>

enum class TankType : char8_t
{
	ENEMY1,
	ENEMY2,
	ENEMY3,
	ENEMY4,
	PLAYER1,
	PLAYER2,
	COOP1,
	COOP2,
	PLAYER3,
	PLAYER4,
	COOP3,
	COOP4
};

[[nodiscard]] constexpr TankType PlayerTankOf(const PlayerSlot slot) noexcept
{
	constexpr std::array kTypes{TankType::PLAYER1, TankType::PLAYER2, TankType::PLAYER3, TankType::PLAYER4};

	return kTypes[SeatIndex(slot)];
}

//NOTE: a bot in a player's seat - it scores into that seat
[[nodiscard]] constexpr TankType CoopTankOf(const PlayerSlot slot) noexcept
{
	constexpr std::array kTypes{TankType::COOP1, TankType::COOP2, TankType::COOP3, TankType::COOP4};

	return kTypes[SeatIndex(slot)];
}

//NOTE: nothing for an enemy
[[nodiscard]] constexpr std::optional<PlayerSlot> SlotOf(const TankType type) noexcept
{
	for (const PlayerSlot slot: kSlots)
	{
		if (type == PlayerTankOf(slot) || type == CoopTankOf(slot))
		{
			return slot;
		}
	}

	return std::nullopt;
}

[[nodiscard]] constexpr bool IsPlayerTank(const TankType type) noexcept
{
	const std::optional<PlayerSlot> slot{SlotOf(type)};

	return slot && type == PlayerTankOf(*slot);
}
