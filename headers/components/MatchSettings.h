#pragma once

#include "enums/MatchRules.h"
#include "enums/PlayerSlot.h"
#include <cstdint>
#include <string>

//NOTE: ENEMY1 to ENEMY4 - the field has no more enemy seats
inline constexpr std::uint8_t kMaxEnemiesAtOnce{4u};
//NOTE: the bots a free-for-all of two at one keyboard keeps on the field - a host picks its own
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
	//NOTE: how many of the seats nobody took bots fill - at most all but one, and a player joining takes a bot's over
	std::uint8_t bots{};
	//NOTE: the match starts with whoever is in, and the rest join it running
	bool isStartingAtOnce{};

	[[nodiscard]] bool operator==(const MatchSettings& rhs) const = default;
};

//NOTE: one seat is always the host's own
[[nodiscard]] constexpr std::uint8_t MaxBots(const std::uint8_t seats) noexcept
{
	return seats > 0u ? static_cast<std::uint8_t>(seats - 1u) : std::uint8_t{};
}

//NOTE: a classic match is played against the enemies
[[nodiscard]] constexpr std::uint8_t FewestEnemies(const MatchRules rules) noexcept
{
	return rules == MatchRules::FreeForAll ? std::uint8_t{} : std::uint8_t{1u};
}
