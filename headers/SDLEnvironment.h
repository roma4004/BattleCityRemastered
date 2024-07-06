#pragma once

#include "Point.h"
#include "interfaces/IConfig.h"

#include <SDL.h>
// #include <SDL_image.h>
#include <SDL_ttf.h>
#include <memory>

struct SDLEnvironment final
{
public:
	SDLEnvironment(UPoint windowSize, const char* fpsFontName);
	~SDLEnvironment();
	std::unique_ptr<IConfig> Init();

	UPoint windowSize{800, 600};
	int* windowBuffer{};
	SDL_Window* window{};
	SDL_Renderer* renderer{};
	SDL_Texture* screen{};

	//fps
	TTF_Font* _fpsFont{nullptr};
	const char* _fpsFontPathName;
};
