#include "application/GameConfig.h"
#include "application/LaunchOptions.h"

void GameConfig::Apply(const LaunchOptions& launchOptions)
{
	gameMode = launchOptions.gameMode;
	skipIntroMusic = launchOptions.skipIntroMusic;
}

UPoint GameConfig::LogicalSize() const { return UPoint{.x = battlefieldSize.x + sideBarWidth, .y = battlefieldSize.y}; }
