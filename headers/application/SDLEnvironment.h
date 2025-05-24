#pragma once

#include <SDL.h>
#include <SDL_mixer.h>
#include <memory>

struct UPoint;
struct Window;
class IConfig;

struct SDLEnvironment final
{
	std::shared_ptr<SDL_Window> sdlWindow{nullptr};
	std::shared_ptr<SDL_Renderer> renderer{nullptr};
	std::shared_ptr<Mix_Chunk> levelStartedSound{nullptr};

	std::shared_ptr<Window> window{nullptr};

	const char* fpsFontPathName{nullptr};
	const char* logoPathName{nullptr};
	const char* introMusicPathName{nullptr};
	const char* textureAtlasPath{nullptr};
	const char* tankEnemyPathName{nullptr};

	bool isVsyncOn{true};//TODO: add input as constructor parameter and export to gameSuccess

	SDLEnvironment(UPoint windowSize, const char* fpsFontName, const char* logoName, const char* introMusicName,
				   const char* textureCollection);

	~SDLEnvironment();

	[[nodiscard]] std::unique_ptr<IConfig> Init();
};