#include "network/commands/GameStateChange.h"

namespace network::commands
{
GameStateChange::GameStateChange(std::string gameState)
	: _gameState{std::move(gameState)} {}

CommandType GameStateChange::GetType() const noexcept { return _type; }

std::string GameStateChange::GetGameState() const noexcept { return _gameState; }

const char* GameStateChange::GetClassNameW() const noexcept { return "GameStateChange"; }
}//namespace network::commands
