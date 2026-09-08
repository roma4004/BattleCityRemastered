#pragma once
#include "components/WorldGeometry.h"
#include "enums/GameMode.h"
#include "enums/GameState.h"
#include <chrono>
#include <cstddef>

struct LaunchOptions;

//NOTE: the live world - every field here changes while the game runs; startup input is ProjectConfig's
class GameConfig final
{
public:
	void Apply(const LaunchOptions& launchOptions);

	//NOTE: here so nothing subscribes to GameModeChangedToEvent merely to read it, written before the reset
	GameMode gameMode{};

	GameState gameState{GameState::Menu};

	[[nodiscard]] bool IsAuthority() const { return ::IsAuthority(gameMode); }
	[[nodiscard]] bool IsClient() const { return ::IsClient(gameMode); }
	[[nodiscard]] bool IsHost() const { return ::IsHost(gameMode); }
	[[nodiscard]] bool HasSecondPlayer() const { return ::HasSecondPlayer(gameMode); }
	[[nodiscard]] UPoint LogicalSize() const;

	UPoint battlefieldSize{WorldGeometry::kClassicBattlefieldSize};
	size_t sideBarWidth{WorldGeometry::kSideBarWidth};
	int tankHealth{100};
	std::chrono::milliseconds enemySpawnCooldown{5000};
	std::chrono::milliseconds bonusLifeTimeCooldown{15000};
	double gridOffset{WorldGeometry::kCellSize};
	double tankSize{gridOffset * 3.0};
	double tankSpeed{142.0};
	int bonusSize{static_cast<int>(tankSize)};
	double botShootObstacleChance{0.35};
	std::chrono::milliseconds botObstacleShootCooldown{1000};
	bool skipIntroMusic{false};//NOTE: launch flag, not persisted - autoplay only, sound stays on
};
