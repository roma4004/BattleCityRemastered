#include "network/commands/GameStateChange.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(network::commands::GameStateChange)

namespace network::commands
{
GameStateChange::GameStateChange()
	: Command{CommandType::GAME_STATE_CHANGE} {}

GameStateChange::GameStateChange(std::string gameState)
	: Command{CommandType::GAME_STATE_CHANGE}
	, _gameState{std::move(gameState)} {}

std::string GameStateChange::GetGameState() const noexcept { return _gameState; }

const char* GameStateChange::GetClassNameW() const noexcept { return "GameStateChange"; }
}//namespace network::commands
