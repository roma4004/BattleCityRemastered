#pragma once
#include "components/WorldGeometry.h"
#include "enums/GameMode.h"
#include "enums/GameState.h"
#include "enums/MatchRules.h"
#include "enums/PlayerSlot.h"
#include "network/Endpoints.h"
#include <chrono>
#include <cstddef>
#include <optional>
#include <string>

struct LaunchOptions;

//NOTE: the live world - every field here changes while the game runs; startup input is ProjectConfig's
class GameConfig final
{
public:
	void Apply(const LaunchOptions& launchOptions);

	//NOTE: here so nothing subscribes to GameModeChangedToEvent merely to read it, written before the reset
	GameMode gameMode{};

	GameState gameState{GameState::Menu};

	std::optional<PlayerSlot> ownSlot{};
	network::ServerAddress serverAddress{};
	//NOTE: the host's; a client learns it with its seat
	std::size_t networkSeats{kDefaultSeats};
	//NOTE: the server's - a client mirrors the match and never needs them
	MatchRules networkRules{};
	//NOTE: how many enemies the field holds at once
	std::size_t simultaneousEnemies{4u};

	[[nodiscard]] bool IsAuthority() const noexcept { return ::IsAuthority(gameMode); }
	[[nodiscard]] bool IsClient() const noexcept { return ::IsClient(gameMode); }
	[[nodiscard]] bool IsHost() const noexcept { return ::IsHost(gameMode); }

	//NOTE: false for both seats on the dedicated server - it drives no tank of its own
	[[nodiscard]] bool IsOwnSlot(const PlayerSlot slot) const
	{
		if (IsHost())
		{
			return false;
		}

		return !IsClient() || ownSlot == slot;
	}
	[[nodiscard]] std::size_t SeatCount() const noexcept
	{
		return IsNetworkGame(gameMode) ? networkSeats : LocalSeats(gameMode);
	}
	[[nodiscard]] MatchRules Rules() const noexcept
	{
		if (IsNetworkGame(gameMode))
		{
			return networkRules;
		}

		return ::IsFreeForAll(gameMode) ? MatchRules::FreeForAll : MatchRules::Classic;
	}
	[[nodiscard]] bool IsFreeForAll() const noexcept { return Rules() == MatchRules::FreeForAll; }
	//NOTE: a free-for-all of several players keeps two bots on the field
	[[nodiscard]] std::size_t EnemySeats() const noexcept
	{
		return IsFreeForAll() && SeatCount() > 1u ? 2u : simultaneousEnemies;
	}
	[[nodiscard]] UPoint LogicalSize() const noexcept;

	UPoint battlefieldSize{WorldGeometry::kClassicBattlefieldSize};
	size_t sideBarWidth{WorldGeometry::kSideBarWidth};
	int tankHealth{100};
	std::chrono::milliseconds enemySpawnCooldown{std::chrono::seconds{5}};
	std::chrono::milliseconds bonusLifeTimeCooldown{std::chrono::seconds{15}};
	double gridOffset{WorldGeometry::kCellSize};
	double tankSize{gridOffset * static_cast<double>(WorldGeometry::kTankCellSpan)};
	double tankSpeed{142.0};
	int bonusSize{static_cast<int>(tankSize)};
	//NOTE: how far a shot may fall either side of the model damage; zero makes every shot identical
	unsigned int bulletDamageSpread{3u};
	//NOTE: a share of the notice bands in InputProviderForBot - zero makes a bot see everything at once
	double botNoticeDelayFactor{1.0};
	double botShootObstacleChance{0.35};
	//NOTE: the base ends the match, and a bot drives past it far less often than past a wall -
	//so it is rolled for on its own, higher chance
	double botShootFortressChance{0.5};
	std::chrono::milliseconds botObstacleShootCooldown{std::chrono::seconds{1}};
	//NOTE: the level the next match loads - the console picks it, and a client never reads a map at
	//all, so the change rides along with the restart that follows it
	std::string mapPath{"Resources/Maps/level1.map"};
	unsigned short stageNumber{1u};
	bool isMuted{};
};
