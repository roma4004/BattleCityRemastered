#include "application/SDLEnvironment.h"
#include "application/ConfigFailure.h"
#include "application/ConfigSuccess.h"
#include "application/UserInput.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <iostream>
#include <memory>

class IConfig;

SDLEnvironment::SDLEnvironment(const UPoint windowSize, const char* fpsFontName, const char* logoName,
							   const char* introMusicName, const char* textureCollection, const char* joyIcon,
							   const char* p1ControlHint, const char* p2ControlHint)
	: windowSize{windowSize}
	, fpsFontPathName{fpsFontName}
	, logoPathName{logoName}
	, introMusicPathName{introMusicName}
	, textureAtlasPath{textureCollection}
	, joyIconPathName{joyIcon}
	, p1ControlHintPathName{p1ControlHint}
	, p2ControlHintPathName{p2ControlHint} {}

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

	// creating window
	UPoint windowSizeHalf{};
	if (sdlWindow = InitWindow(windowSizeHalf);
		sdlWindow == nullptr)
	{
		return std::make_unique<ConfigFailure>("SDL_CreateWindow Error", SDL_GetError());
	}

	// creating renderer
	if (renderer = InitRender(windowSizeHalf);
		renderer == nullptr)
	{
		return std::make_unique<ConfigFailure>("SDL_CreateRenderer Error", SDL_GetError());
	}

	// font init and loading
	std::shared_ptr<TTF_Font> fpsFont{nullptr};
	{
		if (TTF_Init() == -1)
		{
			return std::make_unique<ConfigFailure>("TTF_Init Error", TTF_GetError());
		}

		if (fpsFont = {TTF_OpenFont(fpsFontPathName, 14), TTF_CloseFont};
			fpsFont == nullptr)
		{
			return std::make_unique<ConfigFailure>("TTF font loading Error", TTF_GetError());
		}
	}

	if (!IMG_Init(IMG_INIT_PNG))
	{
		return std::make_unique<ConfigFailure>("IMG_Init Error", IMG_GetError());
	}

	// texture logo loading
	std::shared_ptr<SDL_Texture> logoTexture{nullptr};
	{
		std::shared_ptr<SDL_Surface> logoSurface{nullptr};
		if (logoSurface = {IMG_Load(logoPathName), SDL_FreeSurface};
			logoSurface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG Logo Loading Error", IMG_GetError());
		}

		if (logoTexture = {SDL_CreateTextureFromSurface(renderer.get(), logoSurface.get()), SDL_DestroyTexture};
			logoTexture == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG Logo Texture Creating Error", IMG_GetError());
		}
	}

	// texture joy icon loading
	std::shared_ptr<SDL_Texture> joyIconTexture{nullptr};
	{
		std::shared_ptr<SDL_Surface> joyIconSurface{nullptr};
		if (joyIconSurface = {IMG_Load(joyIconPathName), SDL_FreeSurface};
			joyIconSurface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG Joy icon Loading Error", IMG_GetError());
		}

		if (joyIconTexture = {SDL_CreateTextureFromSurface(renderer.get(), joyIconSurface.get()), SDL_DestroyTexture};
			joyIconTexture == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG Joy icon Texture Creating Error", IMG_GetError());
		}
	}

	// texture P1 controls hint loading
	std::shared_ptr<SDL_Texture> p1ControlHintTexture{nullptr};
	{
		std::shared_ptr<SDL_Surface> p1ControlHintSurface{nullptr};
		if (p1ControlHintSurface = {IMG_Load(p1ControlHintPathName), SDL_FreeSurface};
			p1ControlHintSurface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG p1ControlHint Loading Error", IMG_GetError());
		}

		if (p1ControlHintTexture = {SDL_CreateTextureFromSurface(renderer.get(), p1ControlHintSurface.get()),
									SDL_DestroyTexture};
			p1ControlHintTexture == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG p1ControlHint Texture Creating Error", IMG_GetError());
		}
	}

	// texture P1 controls hint loading
	std::shared_ptr<SDL_Texture> p2ControlHintTexture{nullptr};
	{
		std::shared_ptr<SDL_Surface> p2ControlHintSurface{nullptr};
		if (p2ControlHintSurface = {IMG_Load(p2ControlHintPathName), SDL_FreeSurface};
			p2ControlHintSurface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG p2ControlHint Loading Error", IMG_GetError());
		}

		if (p2ControlHintTexture = {SDL_CreateTextureFromSurface(renderer.get(), p2ControlHintSurface.get()),
									SDL_DestroyTexture};
			p2ControlHintTexture == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG p2ControlHint Texture Creating Error", IMG_GetError());
		}
	}

	// texture atlas loading
	std::shared_ptr<SDL_Texture> atlasTexture{nullptr};
	{
		std::shared_ptr<SDL_Surface> atlasSurface{nullptr};
		if (atlasSurface = {IMG_Load(textureAtlasPath), SDL_FreeSurface};
			atlasSurface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG atlas Surface Loading Error", IMG_GetError());
		}

		const auto rawSurface = atlasSurface.get();
		if (const int result = SDL_SetColorKey(rawSurface, SDL_TRUE, SDL_MapRGB(rawSurface->format, 0, 0, 1));
			result != 0)
		{
			return std::make_unique<ConfigFailure>("IMG atlas SetColorKey Error", SDL_GetError());
		}

		if (atlasTexture = {SDL_CreateTextureFromSurface(renderer.get(), rawSurface), SDL_DestroyTexture};
			atlasTexture == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG atlas Texture Creating Error", IMG_GetError());
		}

		SDL_SetTextureBlendMode(atlasTexture.get(), SDL_BLENDMODE_BLEND);
	}

	// Audio loading and play
	{
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
	}

	return std::make_unique<ConfigSuccess>(windowSize, renderer, fpsFont, logoTexture, atlasTexture, joyIconTexture,
										   p1ControlHintTexture, p2ControlHintTexture, isVsyncOn);
}

[[nodiscard]] std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> SDLEnvironment::InitWindow(
		UPoint& windowSizeHalf) const
{
	const auto title = "Battle City remastered";
	constexpr SDL_WindowFlags windowFlags = SDL_WINDOW_SHOWN;
	const SDL_Rect rect{.x = 100, .y = 100, .w = static_cast<int>(windowSize.x), .h = static_cast<int>(windowSize.y)};
	windowSizeHalf = {.x = static_cast<size_t>(rect.w / 2), .y = static_cast<size_t>(rect.h / 2)};

	return {SDL_CreateWindow(title, rect.x, rect.y, rect.w, rect.h, windowFlags), SDL_DestroyWindow};
}

[[nodiscard]] std::shared_ptr<SDL_Renderer> SDLEnvironment::InitRender(UPoint& windowSizeHalf) const
{
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
							  static_cast<int>(bounds.x + bounds.w / 2 - windowSizeHalf.x / 2),
							  static_cast<int>(bounds.y + bounds.h / 2 - windowSizeHalf.y / 2 - bordersSize.y));
	}

	return {SDL_CreateRenderer(sdlWindow.get(), monitorIndex, renderFlags), SDL_DestroyRenderer};
}
