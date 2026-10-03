#include "application/GameConfig.h"
#include "application/LaunchOptions.h"
#include "components/MatchSettings.h"
#include <cstdint>
#include <filesystem>

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

MatchSettings GameConfig::HostedMatch() const
{
	return MatchSettings{.rules = networkRules,
						 .seats = static_cast<std::uint8_t>(networkSeats),
						 .map = std::filesystem::path{mapPath}.stem().string(),
						 .enemiesAtOnce = static_cast<std::uint8_t>(simultaneousEnemies)};
}
