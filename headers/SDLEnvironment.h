#pragma once

#include "Config.h"

#include <SDL.h>
//#include <SDL2/SDL_image.h>
//#include <SDL2/SDL_ttf.h>
#include <memory>

class SDLEnvironment final
{
public:
	SDLEnvironment(UPoint windowSize);
	~SDLEnvironment();
	std::unique_ptr<Config> Init();

	UPoint windowSize{800, 600};
	int* windowBuffer{};
	SDL_Window* window{};
	SDL_Renderer* renderer{};
	SDL_Texture* screen{};
};
