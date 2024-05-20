#include "../headers/ConfigSuccess.h"

ConfigSuccess::ConfigSuccess(const size_t windowWidth, const size_t windowHeight, int* windowBuffer,
							 SDL_Renderer* renderer, SDL_Texture* screen)
	: _windowWidth{windowWidth}, _windowHeight{windowHeight}, _windowBuffer{windowBuffer}, _renderer{renderer},
	  _screen{screen}
{
}

std::unique_ptr<Game> ConfigSuccess::CreateGame()
{
	return std::make_unique<GameSuccess>(_windowWidth, _windowHeight, _windowBuffer, _renderer, _screen /*, _font*/);
}
