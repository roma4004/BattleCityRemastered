#include "application/GameConfig.h"
#include "application/LaunchOptions.h"
#include "application/ProjectConfig.h"
#include "components/WorldGeometry.h"

GameConfig::GameConfig(const ProjectConfig& projectConfig)
{
	windowSize = UPoint{.x = projectConfig.Get<unsigned>("Window.width", 800u),
						.y = projectConfig.Get<unsigned>("Window.height", 600u)};

	windowPos = UPoint{.x = projectConfig.Get<unsigned>("Window.posX", 100u),
					   .y = projectConfig.Get<unsigned>("Window.posY", 100u)};
}

void GameConfig::Apply(const LaunchOptions& launchOptions)
{
	gameMode = launchOptions.gameMode;

	//NOTE: size first - the host/client offset below is half a window wide
	if (launchOptions.windowSize)
	{
		windowSize = *launchOptions.windowSize;
	}

	if (launchOptions.windowPos)
	{
		windowPos = *launchOptions.windowPos;
		hasExplicitWindowPos = true;
	}

	if (::IsHost(launchOptions.gameMode))
	{
		ApplyWindowOffsetAsHost();
	}
	else if (::IsClient(launchOptions.gameMode))
	{
		ApplyWindowOffsetAsClient();
	}

	skipIntroMusic = launchOptions.skipIntroMusic;
}

void GameConfig::ApplyGeometry(const WorldGeometry& geometry, const std::size_t mapRows)
{
	gridSize = static_cast<float>(mapRows);
	gridOffset = geometry.cellSize;
	sideBarWidth = geometry.sideBarWidth;

	tankSize = gridOffset * 3.f;
	bonusSize = static_cast<int>(tankSize);

	//NOTE: derived from the defaults, never from the current values - scaling the current ones makes
	//every refit compound on the last one, which is how tankSpeed used to drift on each resize
	const float defaultCellSize = static_cast<float>(windowSizeDefault.y) / gridSizeDefault;
	scaleFactor = gridOffset / defaultCellSize;
	tankSpeed = tankSpeedDefault * scaleFactor;
}

void GameConfig::ApplyWindowOffsetAsHost() { windowsPosOffset.x -= windowSize.x / 2; }

void GameConfig::ApplyWindowOffsetAsClient() { windowsPosOffset.x += windowSize.x / 2; }
