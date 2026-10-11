#pragma once

#include "enums/MatchRules.h"
#include "enums/PlayerSlot.h"
#include <cstdint>
#include <string>

//NOTE: ENEMY1 to ENEMY4 - the field has no more enemy seats
inline constexpr std::uint8_t kMaxEnemiesAtOnce{4u};
//NOTE: fewer than in classic - the players of a free-for-all have each other to fight as well
inline constexpr std::uint8_t kFreeForAllBots{2u};

//NOTE: what the host picks before its server starts - the server is launched with them and hands them to every client
struct MatchSettings final
{
	MatchRules rules{};
	std::uint8_t seats{kDefaultSeats};
	//NOTE: a name from the maps folder, the way the console takes it
	std::string map{"level1"};
	//NOTE: none only in a free-for-all - the players then have each other alone, and it takes two to start
	std::uint8_t enemiesAtOnce{kMaxEnemiesAtOnce};
	//NOTE: seats bots hold, up to every one - the empty ones first, then players', who watch until their first command
	std::uint8_t bots{};
	//NOTE: the match starts with whoever is in, and the rest join it running
	bool isStartingAtOnce{};

	[[nodiscard]] bool operator==(const MatchSettings& rhs) const = default;
};

[[nodiscard]] constexpr std::uint8_t MaxBots(const std::uint8_t seats) noexcept { return seats; }

//NOTE: a classic match is played against the enemies
[[nodiscard]] constexpr std::uint8_t FewestEnemies(const MatchRules rules) noexcept
{
	return rules == MatchRules::FreeForAll ? std::uint8_t{} : std::uint8_t{1u};
}

[[nodiscard]] constexpr std::uint8_t DefaultEnemies(const MatchRules rules) noexcept
{
	return rules == MatchRules::FreeForAll ? kFreeForAllBots : kMaxEnemiesAtOnce;
}
