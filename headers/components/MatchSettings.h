#pragma once

#include "enums/MatchRules.h"
#include "enums/PlayerSlot.h"
#include <cstdint>
#include <string>

//NOTE: ENEMY1 to ENEMY4 - the field has no more enemy seats
inline constexpr std::uint8_t kMaxEnemiesAtOnce{4u};
//NOTE: the bots a free-for-all of several players keeps on the field, whatever the enemies at once say
inline constexpr std::uint8_t kFreeForAllBots{2u};

//NOTE: what the host picks before its server starts - the server is launched with them and hands them to every client
struct MatchSettings final
{
	MatchRules rules{};
	std::uint8_t seats{kDefaultSeats};
	//NOTE: a name from the maps folder, the way the console takes it
	std::string map{"level1"};
	//NOTE: classic only - a free-for-all of several players keeps two bots on the field
	std::uint8_t enemiesAtOnce{kMaxEnemiesAtOnce};

	[[nodiscard]] bool operator==(const MatchSettings& rhs) const = default;
};
