#pragma once

#include "Point.h"
#include <SDL.h> //NOTE: do not replace with forward declaration, required for minGW
#include <memory>

struct UPoint;
struct Mix_Chunk;

class IConfig;

struct SDLEnvironment final
{
	std::shared_ptr<SDL_Window> sdlWindow{nullptr};
	std::shared_ptr<SDL_Renderer> renderer{nullptr};
	std::shared_ptr<Mix_Chunk> levelStartedSound{nullptr};

	UPoint windowSize{};

	const char* fpsFontPathName{nullptr};
	const char* logoPathName{nullptr};
	const char* introMusicPathName{nullptr};
	const char* textureAtlasPath{nullptr};

	bool isVsyncOn{false};//TODO: add input as constructor parameter and export to gameSuccess

	SDLEnvironment(UPoint windowSize, const char* fpsFontName, const char* logoName, const char* introMusicName,
	               const char* textureCollection);

	~SDLEnvironment();

	[[nodiscard]] std::unique_ptr<IConfig> Init();
};
