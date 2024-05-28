#include "../headers/ConfigSuccess.h"

ConfigSuccess::ConfigSuccess(const UPoint windowSize, int* windowBuffer, SDL_Renderer* renderer, SDL_Texture* screen)
	: _windowSize{windowSize}, _windowBuffer{windowBuffer}, _renderer{renderer}, _screen{screen}
{
}

std::unique_ptr<Game> ConfigSuccess::CreateGame()
{
	return std::make_unique<GameSuccess>(_windowSize, _windowBuffer, _renderer, _screen /*, _font*/);
}
