#include "application/ConfigSuccess.h"
#include "application/GameSuccess.h"
#include "components/EventSystem.h"
#include "components/Menu.h"
#include "components/RightSideBar.h"
#include "components/managers/RenderManager.h"
#include "enums/GameMode.h"
#include <SDL_ttf.h>

ConfigSuccess::ConfigSuccess(const UPoint windowSize, GameConfig& gameConfig,
							 std::vector<std::shared_ptr<SDL_Texture>>& buttonTexturesPS5,
							 std::vector<std::shared_ptr<SDL_Texture>>& buttonTexturesXBox,
							 const std::shared_ptr<SDL_Renderer>& renderer, const std::shared_ptr<TTF_Font>& fontSmall,
							 const std::shared_ptr<TTF_Font>& fontMedium, const std::shared_ptr<SDL_Texture>& logo,
							 const std::shared_ptr<SDL_Texture>& atlas, const std::shared_ptr<SDL_Texture>& selectorIcon,
							 const bool isVsyncOn)
	: _windowSize{windowSize}
	, gameConfig{gameConfig}
	, buttonTexturesPS5{buttonTexturesPS5}
	, buttonTexturesXBox{buttonTexturesXBox}
	, _renderer{renderer}
	, _fontSmall{fontSmall}
	, _fontMedium{fontMedium}
	, _logo{logo}
	, _atlas{atlas}
	, _selectorIcon{selectorIcon}
	, _isVsyncOn{isVsyncOn} {}

std::unique_ptr<IGame> ConfigSuccess::CreateGame(const GameMode gameMode)
{
	auto events = std::make_shared<EventSystem>();
	auto menu = std::make_unique<Menu>(_windowSize, events);
	auto renderManager = std::make_unique<RenderManager>(events, _renderer, gameConfig, buttonTexturesPS5,
														 buttonTexturesXBox, _fontSmall, _fontMedium, _logo, _atlas,
														 _selectorIcon, _windowSize);
	auto rightSideBar = std::make_unique<RightSideBar>(_windowSize, events);

	return std::make_unique<GameSuccess>(_windowSize, events, menu, _isVsyncOn, renderManager, rightSideBar, gameMode);
}
