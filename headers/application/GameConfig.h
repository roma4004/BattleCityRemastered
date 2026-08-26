#pragma once
#include "components/WorldGeometry.h"
#include "enums/GameMode.h"
#include <chrono>

struct LaunchOptions;
class ProjectConfig;

//NOTE: the live world - every field here changes while the game runs. Startup input (ini, asset
//paths) is ProjectConfig's.
class GameConfig
{
public:
	explicit GameConfig(const ProjectConfig& projectConfig);

	void Apply(const LaunchOptions& launchOptions);
	void ApplyWindowOffsetAsHost();
	void ApplyWindowOffsetAsClient();

	//NOTE: here so nothing subscribes to GameModeChangedToEvent merely to read it. Game writes it
	//before the reset event goes out.
	GameMode gameMode{};

	[[nodiscard]] bool IsAuthority() const { return ::IsAuthority(gameMode); }
	[[nodiscard]] bool IsClient() const { return ::IsClient(gameMode); }
	[[nodiscard]] bool IsHost() const { return ::IsHost(gameMode); }
	[[nodiscard]] bool HasSecondPlayer() const { return ::HasSecondPlayer(gameMode); }
	[[nodiscard]] bool ShouldPersistWindowPos() const { return !hasExplicitWindowPos && !IsHost() && !IsClient(); }
	[[nodiscard]] bool ShouldPersistWindowSize() const { return !hasExplicitWindowSize && !IsHost() && !IsClient(); }
	[[nodiscard]] UPoint LogicalSize() const;

	UPoint windowSize{};
	UPoint windowPos{};
	UPoint windowsPosOffset{};
	UPoint battlefieldSize{WorldGeometry::kClassicBattlefieldSize};
	size_t sideBarWidth{WorldGeometry::kSideBarWidth};
	int tankHealth{100};
	std::chrono::milliseconds enemySpawnCooldown{5000};
	std::chrono::milliseconds bonusLifeTimeCooldown{15000};
	float gridOffset{WorldGeometry::kCellSize};
	float tankSize{gridOffset * 3.f};
	float tankSpeed{142.f};
	int bonusSize{static_cast<int>(tankSize)};
	bool skipIntroMusic{false};//NOTE: launch flag, not persisted - autoplay only, sound stays on
	bool hasExplicitWindowPos{false};//NOTE: explicit pos wins over monitor centering
	bool hasExplicitWindowSize{false};
};
