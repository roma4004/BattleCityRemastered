#pragma once
#include "geometry/Point.h"
#include "enums/GameMode.h"

struct LaunchOptions;
struct WorldGeometry;
class ProjectConfig;

//NOTE: the live world - every field here changes while the game runs. Startup input (ini, asset
//paths) is ProjectConfig's.
class GameConfig
{
public:
	explicit GameConfig(const ProjectConfig& projectConfig);

	void Apply(const LaunchOptions& launchOptions);
	//NOTE: the one place where a cell size turns into every other size - keep derivations out of the spawners
	void ApplyGeometry(const WorldGeometry& geometry, std::size_t mapRows);
	void ApplyWindowOffsetAsHost();
	void ApplyWindowOffsetAsClient();

	//NOTE: here so nothing subscribes to GameModeChangedToEvent merely to read it. Game writes it
	//before the reset event goes out.
	GameMode gameMode{};

	[[nodiscard]] bool IsAuthority() const { return ::IsAuthority(gameMode); }
	[[nodiscard]] bool IsClient() const { return ::IsClient(gameMode); }
	[[nodiscard]] bool IsHost() const { return ::IsHost(gameMode); }
	[[nodiscard]] bool HasSecondPlayer() const { return ::HasSecondPlayer(gameMode); }

	UPoint windowSize{};
	UPoint windowPos{};
	UPoint windowSizeDefault{.x = 800u, .y = 600u};
	UPoint windowsPosOffset{};
	size_t sideBarWidth{175u};
	int tankHealth{100};
	float scaleFactor{1.f};
	//NOTE: gridSize is a count - how many cells fit vertically; gridOffset is one cell in pixels
	float gridSize{50.f};
	float gridSizeDefault{50.f};
	float gridOffset{600 / gridSize};
	float tankSize{gridOffset * 3};
	float tankSpeed{142};
	float tankSpeedDefault{142};
	int bonusSize{static_cast<int>(gridOffset * 3)};
	bool skipIntroMusic{false};//NOTE: launch flag, not persisted - autoplay only, sound stays on
	bool hasExplicitWindowPos{false};//NOTE: explicit pos wins over monitor centering
};
