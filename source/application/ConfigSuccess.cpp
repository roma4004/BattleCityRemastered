#include "application/ConfigSuccess.h"
#include "application/GameSuccess.h"
#include "components/EventSystem.h"
#include "components/GameStatistics.h"
#include "components/Menu.h"
#include "components/managers/BonusEffectManager.h"
#include "components/managers/TextureManager.h"
#include <SDL_ttf.h>

ConfigSuccess::ConfigSuccess(const UPoint windowSize, std::shared_ptr<SDL_Renderer> renderer,
                             std::shared_ptr<TTF_Font> fpsFont, std::shared_ptr<SDL_Texture> logoTexture,
                             std::shared_ptr<SDL_Texture> atlasTexture, const bool isVsyncOn)
	: _windowSize{windowSize},
	  _renderer{std::move(renderer)},
	  _fpsFont{std::move(fpsFont)},
	  _logoTexture{std::move(logoTexture)},
	  _atlasTexture{std::move(atlasTexture)},
	  _isVsyncOn{isVsyncOn} {}

std::unique_ptr<IGame> ConfigSuccess::CreateGame()
{
	auto events = std::make_shared<EventSystem>();
	auto statistics = std::make_shared<GameStatistics>(events);
	auto menu = std::make_unique<Menu>(_renderer, _fpsFont, _logoTexture, statistics, _windowSize, events);
	auto textureManager = std::make_shared<TextureManager>(_windowSize, _atlasTexture, _renderer, _fpsFont, events);
	auto bonusEffectManager = std::make_shared<BonusEffectManager>(events);

	return std::make_unique<GameSuccess>(
			_windowSize, events, statistics, std::move(menu), textureManager, _isVsyncOn, bonusEffectManager);
}
