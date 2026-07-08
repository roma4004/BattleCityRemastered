#pragma once

#include "Point.h"
#include <SDL.h>//NOTE: do not replace with forward declaration, required for minGW
#include <SDL_mixer.h>
#include <memory>
#include <boost/property_tree/ptree.hpp>

struct UPoint;

class IConfig;

struct SDLEnvironment final
{
	std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> sdlWindow{nullptr, nullptr};
	std::unique_ptr<Mix_Chunk, decltype(&Mix_FreeChunk)> levelStartedSound{nullptr, nullptr};
	std::shared_ptr<SDL_Renderer> renderer{nullptr};

	UPoint& windowSize;
	UPoint& windowPos;
	UPoint windowsPosOffset;

	boost::property_tree::ptree& pTreeIni;
	std::vector<std::shared_ptr<SDL_Texture>> buttonTexturesPS5;
	std::vector<std::shared_ptr<SDL_Texture>> buttonTexturesXBox;

	SDLEnvironment(UPoint& windowSize, UPoint& windowPos, const UPoint& windowsPosOffset, boost::property_tree::ptree& pTreeIni);
	~SDLEnvironment();

	[[nodiscard]] std::unique_ptr<IConfig> Init();

	[[nodiscard]] std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> InitWindow() const;
	[[nodiscard]] std::shared_ptr<SDL_Renderer> InitRender() const;
};
