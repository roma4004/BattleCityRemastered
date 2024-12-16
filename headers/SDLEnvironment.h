#pragma once

#include "Point.h"
#include "interfaces/IConfig.h"

#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <memory>

struct SDLEnvironment final
{
public:
	SDLEnvironment(UPoint windowSize, const char* fpsFontName, const char* logoName, const char* levelStartedName);
	~SDLEnvironment();
	std::unique_ptr<IConfig> Init();

	UPoint windowSize{.x = 800, .y = 600};
	int* windowBuffer{};
	SDL_Window* window{};
	SDL_Renderer* renderer{};
	SDL_Texture* screen{};


	//fps
	TTF_Font* _fpsFont{nullptr};
	const char* _fpsFontPathName{nullptr};

	SDL_Texture* _logoTexture{nullptr};
	const char* _logoPathName{nullptr};
	const char* _levelStartedPathName{nullptr};
	Mix_Chunk* _levelStartedSound{nullptr};
};
