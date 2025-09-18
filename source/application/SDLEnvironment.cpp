#include "application/SDLEnvironment.h"
#include "application/ConfigFailure.h"
#include "application/ConfigSuccess.h"
#include "application/UserInput.h"
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <SDL.h>
#include <SDL_gamecontroller.h>
#include <iostream>
#include <memory>

class IConfig;

SDLEnvironment::SDLEnvironment(const UPoint windowSize, const char* fpsFontName, const char* logoName,
                               const char* introMusicName, const char* textureCollection)
	: windowSize{windowSize},
	  fpsFontPathName{fpsFontName},
	  logoPathName{logoName},
	  introMusicPathName{introMusicName},
	  textureAtlasPath{textureCollection} {}

SDLEnvironment::~SDLEnvironment()
{
	Mix_CloseAudio();
	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}

[[nodiscard]] std::unique_ptr<IConfig> SDLEnvironment::Init()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		return std::make_unique<ConfigFailure>("SDL_Init Error: ", SDL_GetError());
	}

	const auto title = "Battle City remastered";
	constexpr auto windowFlags = SDL_WINDOW_SHOWN;
	const SDL_Rect rect{100, 100, static_cast<int>(windowSize.x), static_cast<int>(windowSize.y)};
	sdlWindow = {SDL_CreateWindow(title, rect.x, rect.y, rect.w, rect.h, windowFlags), SDL_DestroyWindow};
	if (sdlWindow == nullptr)
	{
		return std::make_unique<ConfigFailure>("SDL_CreateWindow Error", SDL_GetError());
	}

	Uint32 renderFlags = SDL_RENDERER_ACCELERATED;
	if (isVsyncOn)
	{
		renderFlags |= SDL_RENDERER_PRESENTVSYNC;
	}

	constexpr int monitorIndex = -1;//NOTE: -1 mean use the default//TODO: move to userSettings

	SDL_Rect bounds;
	SDL_GetDisplayBounds(monitorIndex, &bounds);

	SDL_Rect bordersSize;
	SDL_GetWindowBordersSize(sdlWindow.get(), &bordersSize.y, &bordersSize.x, &bordersSize.h, &bordersSize.w);

	if constexpr (monitorIndex != -1)
	{
		SDL_SetWindowPosition(sdlWindow.get(),
		                      bounds.x + bounds.w / 2 - rect.w / 2,
		                      bounds.y + bounds.h / 2 - rect.h / 2 - bordersSize.y);
	}

	renderer = {SDL_CreateRenderer(sdlWindow.get(), monitorIndex, renderFlags), SDL_DestroyRenderer};
	if (renderer == nullptr)
	{
		return std::make_unique<ConfigFailure>("SDL_CreateRenderer Error", SDL_GetError());
	}

	// font loading
	if (TTF_Init() == -1)
	{
		return std::make_unique<ConfigFailure>("TTF_Init Error", TTF_GetError());
	}

	const std::shared_ptr<TTF_Font> fpsFont{TTF_OpenFont(fpsFontPathName, 14), TTF_CloseFont};
	if (fpsFont == nullptr)
	{
		return std::make_unique<ConfigFailure>("TTF font loading Error", TTF_GetError());
	}

	// texture logo loading
	if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
	{
		return std::make_unique<ConfigFailure>("IMG_Init Error", IMG_GetError());
	}

	const std::shared_ptr<SDL_Surface> logoSurface{IMG_Load(logoPathName), SDL_FreeSurface};
	if (logoSurface == nullptr)
	{
		return std::make_unique<ConfigFailure>("IMG Logo Loading Error", IMG_GetError());
	}

	const std::shared_ptr<SDL_Texture> logoTexture{SDL_CreateTextureFromSurface(renderer.get(), logoSurface.get()),
	                                               SDL_DestroyTexture};
	if (logoTexture == nullptr)
	{
		return std::make_unique<ConfigFailure>("IMG Logo Texture Creating Error", IMG_GetError());
	}

	// texture atlas loading
	const std::shared_ptr<SDL_Surface> atlasSurface{IMG_Load(textureAtlasPath), SDL_FreeSurface};
	if (atlasSurface == nullptr)
	{
		return std::make_unique<ConfigFailure>("IMG atlas Surface Loading Error", IMG_GetError());
	}

	const auto rawSurface = atlasSurface.get();
	if (const int result = SDL_SetColorKey(rawSurface, SDL_TRUE, SDL_MapRGB(rawSurface->format, 0, 0, 1));
		result != 0)
	{
		return std::make_unique<ConfigFailure>("IMG atlas SetColorKey Error", SDL_GetError());
	}

	const std::shared_ptr<SDL_Texture> atlasTexture{SDL_CreateTextureFromSurface(renderer.get(), rawSurface),
	                                                SDL_DestroyTexture};
	if (atlasTexture == nullptr)
	{
		return std::make_unique<ConfigFailure>("IMG atlas Texture Creating Error", IMG_GetError());
	}
	
	SDL_SetTextureBlendMode(atlasTexture.get(), SDL_BLENDMODE_BLEND);

	// Audio loading
	if (const int result = Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
		result < 0)
	{
		return std::make_unique<ConfigFailure>("Mix_OpenAudio Error", Mix_GetError());
	}

	if (levelStartedSound = {Mix_LoadWAV(introMusicPathName), Mix_FreeChunk};
		levelStartedSound == nullptr)
	{
		return std::make_unique<ConfigFailure>("Mix_LoadWAV levelStarted.wav load Error", Mix_GetError());
	}

	if (const int result = Mix_PlayChannel(-1, levelStartedSound.get(), 0);
		result == -1)
	{
		return std::make_unique<ConfigFailure>("Mix_PlayChannel levelStarted.wav play Error", Mix_GetError());
	}

	// Gamepads initialization
	std::cout<< SDL_NumJoysticks() << " gamepad/s connected\n";
	
		int device_index = 0;
	
		SDL_JoystickOpen(device_index);
		if (SDL_GameControllerOpen(device_index))  
		{
			GameControllerOne = SDL_GameControllerOpen(device_index);
			if (GameControllerOne)
			{SDL_Log("Opened controller one: %s", SDL_GameControllerName(GameControllerOne));} 
		}
		++device_index;
		SDL_JoystickOpen(device_index);
		if (SDL_GameControllerOpen(device_index))
		{
			GameControllerTwo = SDL_GameControllerOpen(device_index);
			if (GameControllerTwo)
			{SDL_Log("Opened controller two: %s", SDL_GameControllerName(GameControllerTwo));} 
		}
		
	return std::make_unique<ConfigSuccess>(windowSize, renderer, fpsFont, logoTexture, atlasTexture, isVsyncOn);
}
