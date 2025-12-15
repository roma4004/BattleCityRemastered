#include "application/ConfigSuccess.h"
#include "application/GameSuccess.h"
#include "components/EventSystem.h"
#include "components/GameStatistics.h"
#include "components/Menu.h"
#include "components/managers/TextureManager.h"
#include <SDL_ttf.h>

ConfigSuccess::ConfigSuccess(const UPoint windowSize, const std::shared_ptr<SDL_Renderer>& renderer,
                             const std::shared_ptr<TTF_Font>& fpsFont, const std::shared_ptr<SDL_Texture>& logoTexture,
                             const std::shared_ptr<SDL_Texture>& atlasTexture, const bool isVsyncOn)
	: _windowSize{windowSize},
	  _renderer{renderer},
	  _fpsFont{fpsFont},
	  _logoTexture{logoTexture},
	  _atlasTexture{atlasTexture},
	  _isVsyncOn{isVsyncOn} {}

std::unique_ptr<IGame> ConfigSuccess::CreateGame()
{
	auto events = std::make_shared<EventSystem>();
	auto statistics = std::make_shared<GameStatistics>(events);
	auto menu = std::make_unique<Menu>(_renderer, _fpsFont, _logoTexture, statistics, _windowSize, events);
	auto animationManager = std::make_shared<AnimationManager>(events);
	auto textureManager = std::make_shared<TextureManager>(
			_windowSize, _atlasTexture, _renderer, _fpsFont, events, animationManager);

	return std::make_unique<GameSuccess>(_windowSize, events, statistics, std::move(menu), textureManager, _isVsyncOn);
}
