#include "../headers/ConfigSuccess.h"
#include "../headers/EventSystem.h"
#include "../headers/GameStatistics.h"
#include "../headers/GameSuccess.h"

ConfigSuccess::ConfigSuccess(const UPoint windowSize, int* windowBuffer, SDL_Renderer* renderer, SDL_Texture* screen,
                             TTF_Font* fpsFont, SDL_Texture* logoTexture)
	: _windowSize{windowSize},
	  _windowBuffer{windowBuffer},
	  _renderer{renderer},
	  _screen{screen},
	  _fpsFont{fpsFont},
	  _logoTexture{logoTexture} {}

std::unique_ptr<IGame> ConfigSuccess::CreateGame()
{
	auto events = std::make_shared<EventSystem>();
	auto inputProvider = std::make_unique<InputProviderForMenu>("MenuInput", events);
	auto statistics = std::make_shared<GameStatistics>(events);
	return std::make_unique<GameSuccess>(_windowSize, _windowBuffer, _renderer, _screen, _fpsFont, _logoTexture, events,
	                                     inputProvider, statistics);
}
