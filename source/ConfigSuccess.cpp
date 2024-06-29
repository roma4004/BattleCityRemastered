#include "../headers/ConfigSuccess.h"
#include "../headers/GameSuccess.h"

ConfigSuccess::ConfigSuccess(const UPoint windowSize, int* windowBuffer, SDL_Renderer* renderer, SDL_Texture* screen,
                             TTF_Font* fpsFont)
	: _windowSize{windowSize}, _windowBuffer{windowBuffer}, _renderer{renderer}, _screen{screen}, _fpsFont{fpsFont} {}

std::unique_ptr<IGame> ConfigSuccess::CreateGame()
{
	auto events = std::make_shared<EventSystem>();
	auto inputProvider = std::make_unique<InputProviderForMenu>("menu", events);
	auto statistics = std::make_unique<GameStatistics>(events);
	return std::make_unique<GameSuccess>(_windowSize, _windowBuffer, _renderer, _screen, _fpsFont, events,
	                                     inputProvider, statistics);
}
