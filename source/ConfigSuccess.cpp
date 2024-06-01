#include "../headers/ConfigSuccess.h"

ConfigSuccess::ConfigSuccess(const UPoint windowSize, int* windowBuffer, SDL_Renderer* renderer, SDL_Texture* screen,
                             TTF_Font* fpsFont)
	: _windowSize{windowSize}, _windowBuffer{windowBuffer}, _renderer{renderer}, _screen{screen}, _fpsFont{fpsFont} {}

std::unique_ptr<Game> ConfigSuccess::CreateGame()
{
	return std::make_unique<GameSuccess>(_windowSize, _windowBuffer, _renderer, _screen, _fpsFont);
}
