#include "application/GameConfig.h"
#include "application/LaunchOptions.h"
#include "application/ProjectConfig.h"

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
		hasExplicitWindowSize = true;
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

void GameConfig::ApplyWindowOffsetAsHost() { windowsPosOffset.x -= windowSize.x / 2; }

void GameConfig::ApplyWindowOffsetAsClient() { windowsPosOffset.x += windowSize.x / 2; }

UPoint GameConfig::LogicalSize() const { return UPoint{.x = battlefieldSize.x + sideBarWidth, .y = battlefieldSize.y}; }
