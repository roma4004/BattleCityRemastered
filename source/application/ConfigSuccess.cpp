#include "application/ConfigSuccess.h"
#include "application/GameSuccess.h"
#include "components/EventSystem.h"
#include "components/Menu.h"
#include "components/managers/RenderManager.h"
#include <SDL_ttf.h>

ConfigSuccess::ConfigSuccess(const UPoint windowSize, const std::shared_ptr<SDL_Renderer>& renderer,
							 const std::shared_ptr<TTF_Font>& font, const std::shared_ptr<SDL_Texture>& logo,
							 const std::shared_ptr<SDL_Texture>& atlas, const std::shared_ptr<SDL_Texture>& joyIcon,
							 const std::shared_ptr<SDL_Texture>& xBoxHint, const std::shared_ptr<SDL_Texture>& pS5Hint,
							 const bool isVsyncOn)
	: _windowSize{windowSize}
	, _renderer{renderer}
	, _font{font}
	, _logo{logo}
	, _atlas{atlas}
	, _joyIcon{joyIcon}
	, _xBoxHint{xBoxHint}
	, _pS5Hint{pS5Hint}
	, _isVsyncOn{isVsyncOn} {}

std::unique_ptr<IGame> ConfigSuccess::CreateGame()
{
	auto events = std::make_shared<EventSystem>();
	auto menu = std::make_unique<Menu>(_windowSize, events);
	auto renderManager = std::make_unique<RenderManager>(
			events, _renderer, _font, _logo, _atlas, _joyIcon, _xBoxHint, _pS5Hint, _windowSize);
	auto rightSideBar = std::make_unique<RightSideBar>(_windowSize, events);

	return std::make_unique<GameSuccess>(_windowSize, events, menu,
										 _isVsyncOn, renderManager, rightSideBar);
}
