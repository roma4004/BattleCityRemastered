#include "application/ConfigSuccess.h"
#include "application/GameConfig.h"
#include "application/GameSuccess.h"
#include "components/EventSystem.h"
#include "components/Menu.h"
#include "components/RightSideBar.h"
#include "components/managers/RenderManager.h"
#include "enums/GameMode.h"

ConfigSuccess::ConfigSuccess(GameConfig& gameConfig)
	: _gameConfig{gameConfig} {}

std::unique_ptr<IGame> ConfigSuccess::CreateGame(const GameMode gameMode)
{
	auto events = std::make_shared<EventSystem>();
	auto menu = std::make_unique<Menu>(_gameConfig.windowSize, events);
	auto renderManager = std::make_unique<RenderManager>(events, _gameConfig);
	auto rightSideBar = std::make_unique<RightSideBar>(events);

	return std::make_unique<GameSuccess>(_gameConfig, events, menu, renderManager, rightSideBar, gameMode);
}
