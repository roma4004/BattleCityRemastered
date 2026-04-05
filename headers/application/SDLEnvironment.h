#pragma once

#include "Point.h"
#include <SDL.h>//NOTE: do not replace with forward declaration, required for minGW
#include <SDL_mixer.h>
#include <memory>

struct UPoint;

class IConfig;

struct SDLEnvironment final
{
	std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> sdlWindow{nullptr, nullptr};
	std::unique_ptr<Mix_Chunk, decltype(&Mix_FreeChunk)> levelStartedSound{nullptr, nullptr};
	std::shared_ptr<SDL_Renderer> renderer{nullptr};

	UPoint windowSize{};

	const char* fpsFontPathName{nullptr};
	const char* logoPathName{nullptr};
	const char* introMusicPathName{nullptr};
	const char* textureAtlasPath{nullptr};
	const char* joyIconPathName{nullptr};

	bool isVsyncOn{false};//TODO: add input as constructor parameter and export to gameSuccess

	SDLEnvironment(UPoint windowSize, const char* fpsFontName, const char* logoName, const char* introMusicName,
				   const char* textureCollection, const char* joyIcon);

	~SDLEnvironment();

	[[nodiscard]] std::unique_ptr<IConfig> Init();

	std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> InitWindow(UPoint& windowSizeHalf) const;
	std::shared_ptr<SDL_Renderer> InitRender(UPoint& windowSizeHalf) const;
};
