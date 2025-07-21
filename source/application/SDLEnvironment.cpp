#include "application/SDLEnvironment.h"
#include "application/ConfigFailure.h"
#include "application/ConfigSuccess.h"
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
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

	if (TTF_Init() == -1)
	{
		return std::make_unique<ConfigFailure>("TTF_Init Error", TTF_GetError());
	}

	const std::shared_ptr<TTF_Font> fpsFont{TTF_OpenFont(fpsFontPathName, 14), TTF_CloseFont};
	if (fpsFont == nullptr)
	{
		return std::make_unique<ConfigFailure>("TTF font loading Error", TTF_GetError());
	}

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
	                                               SDL_DestroyTexture};//TODO: replace to logo from texture atlas
	if (logoTexture == nullptr)
	{
		return std::make_unique<ConfigFailure>("IMG Logo Texture Creating Error", IMG_GetError());
	}

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
	{
		return std::make_unique<ConfigFailure>("Mix_OpenAudio Error", Mix_GetError());
	}

	if (levelStartedSound = {Mix_LoadWAV(introMusicPathName), Mix_FreeChunk}; levelStartedSound)
	{
		if (Mix_PlayChannel(-1, levelStartedSound.get(), 0) == -1)
		{
			return std::make_unique<ConfigFailure>("Mix_PlayChannel levelStarted.wav play Error", Mix_GetError());
		}
	}
	else
	{
		return std::make_unique<ConfigFailure>("Mix_LoadWAV levelStarted.wav load Error", Mix_GetError());
	}

	std::shared_ptr<SDL_Surface> atlasSurface{IMG_Load(textureAtlasPath), SDL_FreeSurface};
	if (atlasSurface == nullptr)
	{
		return std::make_unique<ConfigFailure>("IMG atlas Surface Loading Error", IMG_GetError());
	}

	std::ignore = SDL_SetColorKey(atlasSurface.get(), SDL_TRUE, SDL_MapRGB(atlasSurface.get()->format, 0, 0, 1));

	std::shared_ptr<SDL_Texture> atlasTexture{SDL_CreateTextureFromSurface(renderer.get(), atlasSurface.get()),
	                                          SDL_DestroyTexture};
	if (atlasTexture == nullptr)
	{
		return std::make_unique<ConfigFailure>("IMG atlas Texture Creating Error", IMG_GetError());
	}

	SDL_SetTextureBlendMode(atlasTexture.get(), SDL_BLENDMODE_BLEND);

	return std::make_unique<ConfigSuccess>(windowSize, renderer, fpsFont, logoTexture, atlasTexture, isVsyncOn);
}
