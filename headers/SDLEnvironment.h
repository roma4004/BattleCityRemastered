#pragma once

#include "Config.h"

#include <SDL.h>
//#include <SDL2/SDL_image.h>
//#include <SDL2/SDL_ttf.h>
#include <memory>

class SDLEnvironment final
{
public:
	SDLEnvironment(int windowWidth, int windowHeight);
	~SDLEnvironment();
	std::unique_ptr<Config> Init();

	int windowWidth = 800;
	int windowHeight = 600;
	int* windowBuffer{};
	SDL_Window* window{};
	SDL_Renderer* renderer{};
	SDL_Texture* screen{};
};
