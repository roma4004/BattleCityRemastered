#include "application/GameConfig.h"
#include "application/LaunchOptions.h"

void GameConfig::Apply(const LaunchOptions& launchOptions)
{
	gameMode = launchOptions.gameMode;
	isMuted = launchOptions.isMuted;
	serverAddress.host = launchOptions.serverHost.value_or(serverAddress.host);
	serverAddress.port = launchOptions.serverPort.value_or(serverAddress.port);
}

UPoint GameConfig::LogicalSize() const noexcept
{
	return UPoint{.x = battlefieldSize.x + sideBarWidth, .y = battlefieldSize.y};
}
