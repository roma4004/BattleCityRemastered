#pragma once

#include "enums/PlayerSlot.h"
#include <array>

//NOTE: what one seat did, and what was done to it
struct SeatStatistics final
{
	unsigned short bulletHits{};
	unsigned short enemyHits{};
	unsigned short enemyKills{};
	unsigned short hitByEnemyTeam{};
	unsigned short friendlyHitsTaken{};
	unsigned short friendlyKillsTaken{};
	unsigned short brickWallKills{};
	unsigned short steelWallKills{};
	unsigned short bonusPickups{};
	unsigned short bonusesDestroyed{};

	[[nodiscard]] bool operator==(const SeatStatistics& rhs) const = default;
};

//NOTE: the enemy team as a whole
struct EnemyTeamStatistics final
{
	unsigned short bulletHits{};
	unsigned short playerKills{};
	unsigned short friendlyHitsTaken{};
	unsigned short friendlyKillsTaken{};
	unsigned short brickWallKills{};
	unsigned short steelWallKills{};
	unsigned short bonusPickups{};
	unsigned short bonusesDestroyed{};

	[[nodiscard]] bool operator==(const EnemyTeamStatistics& rhs) const = default;
};

struct StatisticsData final
{
	std::array<SeatStatistics, kSeatCount> seats{};
	EnemyTeamStatistics enemyTeam{};
	unsigned short bonusExpired{};

	[[nodiscard]] bool operator==(const StatisticsData& rhs) const = default;

	[[nodiscard]] constexpr const SeatStatistics& Seat(const PlayerSlot slot) const noexcept
	{
		return seats[SeatIndex(slot)];
	}
};
