#include "../../headers/application/ConfigSuccess.h"
#include "../../headers/application/GameSuccess.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/components/GameStatistics.h"
#include "../../headers/components/Menu.h"

ConfigSuccess::ConfigSuccess(std::shared_ptr<Window> window, std::shared_ptr<SDL_Renderer> renderer,
                             std::shared_ptr<SDL_Texture> screen, std::shared_ptr<TTF_Font> fpsFont,
                             std::shared_ptr<SDL_Texture> logoTexture)
	: _window{std::move(window)},
	  _renderer{std::move(renderer)},
	  _screen{std::move(screen)},
	  _fpsFont{std::move(fpsFont)},
	  _logoTexture{std::move(logoTexture)} {}

std::unique_ptr<IGame> ConfigSuccess::CreateGame()
{
	auto events = std::make_shared<EventSystem>();
	auto statistics = std::make_shared<GameStatistics>(events);
	auto menu = std::make_unique<Menu>(_renderer, _fpsFont, _logoTexture, statistics, _window, events);

	return std::make_unique<GameSuccess>(_window, _renderer, _screen, _fpsFont, events, statistics, std::move(menu));
}
