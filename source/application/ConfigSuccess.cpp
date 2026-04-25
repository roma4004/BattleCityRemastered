#include "application/ConfigSuccess.h"
#include "application/GameSuccess.h"
#include "components/EventSystem.h"
#include "components/Menu.h"
#include "components/managers/RenderManager.h"
#include <SDL_ttf.h>

ConfigSuccess::ConfigSuccess(const UPoint windowSize, const std::shared_ptr<SDL_Renderer>& renderer,
							 const std::shared_ptr<TTF_Font>& fpsFont, const std::shared_ptr<SDL_Texture>& logoTexture,
							 const std::shared_ptr<SDL_Texture>& atlasTexture,
							 const std::shared_ptr<SDL_Texture>& joyIcon,
							 const std::shared_ptr<SDL_Texture>& p1ControlHint,
							 const std::shared_ptr<SDL_Texture>& p2ControlHint,
							 const bool isVsyncOn)
	: _windowSize{windowSize}
	, _renderer{renderer}
	, _fpsFont{fpsFont}
	, _logoTexture{logoTexture}
	, _atlasTexture{atlasTexture}
	, _joyIcon{joyIcon}
	, _p1ControlHint{p1ControlHint}
	, _p2ControlHint{p2ControlHint}
	, _isVsyncOn{isVsyncOn} {}

std::unique_ptr<IGame> ConfigSuccess::CreateGame()
{
	auto events = std::make_shared<EventSystem>();
	auto menu = std::make_unique<Menu>(_windowSize, events);
	auto renderManager = std::make_unique<RenderManager>(events, _renderer, _fpsFont, _logoTexture, _atlasTexture,
														 _joyIcon, _p1ControlHint, _p2ControlHint, _windowSize);

	return std::make_unique<GameSuccess>(_windowSize, events, menu, _isVsyncOn, renderManager);
}
