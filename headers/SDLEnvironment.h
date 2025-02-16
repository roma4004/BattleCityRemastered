#pragma once

#include "Point.h"
#include "interfaces/IConfig.h"

#include <SDL.h>
#include <SDL_mixer.h>
#include <memory>

struct SDLEnvironment final
{
	SDLEnvironment(UPoint windowSize, const char* fpsFontName, const char* logoName, const char* introMusicName);

	~SDLEnvironment();

	std::unique_ptr<IConfig> Init();
	std::shared_ptr<SDL_Window> window{nullptr};
	std::shared_ptr<SDL_Renderer> renderer{nullptr};
	std::shared_ptr<Mix_Chunk> levelStartedSound{nullptr};

	UPoint windowSize{.x = 800, .y = 600}; //TODO: combine windowSize and window buffer into structure to passing in methods easier

	const char* fpsFontPathName{nullptr};
	const char* logoPathName{nullptr};
	const char* introMusicPathName{nullptr};
};
