#include "../../headers/application/SDLEnvironment.h"
#include "../../headers/application/ConfigFailure.h"
#include "../../headers/application/ConfigSuccess.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <memory>

class IConfig;

SDLEnvironment::SDLEnvironment(UPoint windowSize, const char* fpsFontName, const char* logoName,
                               const char* introMusicName)
	: fpsFontPathName{fpsFontName},
	  logoPathName{logoName},
	  introMusicPathName{introMusicName}
{
	const auto size = windowSize.x * windowSize.y;
	window = std::make_shared<Window>(std::move(windowSize),
	                                  std::shared_ptr<int[]>(new int[size], std::default_delete<int[]>()));
}

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

	sdlWindow = std::shared_ptr<SDL_Window>(
			SDL_CreateWindow("Battle City remastered", 100, 100, static_cast<int>(window->size.x),
			                 static_cast<int>(window->size.y), SDL_WINDOW_SHOWN),
			SDL_DestroyWindow);
	if (sdlWindow == nullptr)
	{
		return std::make_unique<ConfigFailure>("SDL_CreateWindow Error", SDL_GetError());
	}

	renderer = std::shared_ptr<SDL_Renderer>(
			SDL_CreateRenderer(sdlWindow.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
			SDL_DestroyRenderer);
	if (renderer == nullptr)
	{
		return std::make_unique<ConfigFailure>("SDL_CreateRenderer Error", SDL_GetError());
	}

	const std::shared_ptr<SDL_Texture> screen(
			SDL_CreateTexture(renderer.get(), SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET,
			                  static_cast<int>(window->size.x), static_cast<int>(window->size.y)),
			SDL_DestroyTexture);
	if (screen == nullptr)
	{
		return std::make_unique<ConfigFailure>("Screen SDL_CreateTexture Error", SDL_GetError());
	}

	if (TTF_Init() == -1)
	{
		return std::make_unique<ConfigFailure>("TTF_Init Error", TTF_GetError());
	}
	std::shared_ptr<TTF_Font> fpsFont(TTF_OpenFont(fpsFontPathName, 14), TTF_CloseFont);
	if (fpsFont == nullptr)
	{
		return std::make_unique<ConfigFailure>("TTF font loading Error", TTF_GetError());
	}

	if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
	{
		return std::make_unique<ConfigFailure>("IMG_Init Error", IMG_GetError());
	}

	const std::shared_ptr<SDL_Surface> logoSurface(IMG_Load(logoPathName), SDL_FreeSurface);
	if (logoSurface == nullptr)
	{
		return std::make_unique<ConfigFailure>("IMG Logo Loading Error", IMG_GetError());
	}

	std::shared_ptr<SDL_Texture> logoTexture(SDL_CreateTextureFromSurface(renderer.get(), logoSurface.get()),
	                                         SDL_DestroyTexture);
	if (logoTexture == nullptr)
	{
		return std::make_unique<ConfigFailure>("IMG Logo Texture Creating Error", IMG_GetError());
	}

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
	{
		return std::make_unique<ConfigFailure>("Mix_OpenAudio Error", Mix_GetError());
	}

	levelStartedSound = std::shared_ptr<Mix_Chunk>(Mix_LoadWAV(introMusicPathName), Mix_FreeChunk);
	if (levelStartedSound == nullptr)
	{
		return std::make_unique<ConfigFailure>("Mix_LoadWAV levelStarted.wav load Error", Mix_GetError());
	}

	if (Mix_PlayChannel(-1, levelStartedSound.get(), 0) == -1)
	{
		return std::make_unique<ConfigFailure>("Mix_PlayChannel levelStarted.wav play Error", Mix_GetError());
	}

	return std::make_unique<ConfigSuccess>(window, renderer, screen, fpsFont, logoTexture);
}
