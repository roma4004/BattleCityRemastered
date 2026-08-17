#include "application/SDL_Config.h"
#include "application/GameConfig.h"
#include "application/UserInput.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <array>
#include <iostream>
#include <memory>

namespace
{
//NOTE: positional - RenderManager draws these vectors by index, so the order is part of the contract
constexpr std::array kPs5Keys{"Images.PS5_Create", "Images.PS5_Cross", "Images.PS5_D-Pad",
							  "Images.PS5_Home", "Images.PS5_Options", "Images.PS5_Triangle"};
constexpr std::array kXBoxKeys{"Images.XBox_D-Pad", "Images.XBox_Home", "Images.XBox_Menu",
							   "Images.XBox_View", "Images.XBox_A", "Images.XBox_Y"};
}

SDL_Config::SDL_Config(GameConfig& config)
	: gameConfig{config} {}

SDL_Config::~SDL_Config()
{
	Mix_CloseAudio();
	fontSmall.reset();
	fontMedium.reset();
	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}

std::expected<void, InitError> SDL_Config::Init()
{
	//NOTE: every group fills this object's fields; and_then stops at the first one that refuses, so
	//no later group ever runs on a half-built environment
	return InitVideo()
			.and_then([this] { return InitFonts(); })
			.and_then([this] { return InitTextures(); })
			.transform([this]
			{
				//NOTE: the game runs without sound, so this one failure is reported and dropped. The
				//decision to ignore it belongs here, at the call site - not inside InitAudio, which
				//has no business deciding how much its own failure matters.
				if (const auto audio = InitAudio();
					!audio)
				{
					std::cerr << audio.error().stage << ": " << audio.error().detail << '\n';
				}
			});
}

std::expected<void, InitError> SDL_Config::InitVideo()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		return std::unexpected(InitError{.stage = "SDL_Init Error", .detail = SDL_GetError()});
	}

	if (sdlWindow = InitWindow();
		sdlWindow == nullptr)
	{
		return std::unexpected(InitError{.stage = "SDL_CreateWindow Error", .detail = SDL_GetError()});
	}

	if (renderer = InitRender();
		renderer == nullptr)
	{
		return std::unexpected(InitError{.stage = "SDL_CreateRenderer Error", .detail = SDL_GetError()});
	}

	SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_BLEND);

	return {};
}

std::expected<void, InitError> SDL_Config::InitFonts()
{
	if (TTF_Init() == -1)
	{
		return std::unexpected(InitError{.stage = "TTF_Init Error", .detail = TTF_GetError()});
	}

	const std::string fontPath = PathFromConfig("Fonts.BattleCity");

	if (fontSmall = {TTF_OpenFont(fontPath.c_str(), 14), TTF_CloseFont};
		fontSmall == nullptr)
	{
		return std::unexpected(InitError{.stage = "TTF font loading Error", .detail = TTF_GetError()});
	}

	if (fontMedium = {TTF_OpenFont(fontPath.c_str(), 24), TTF_CloseFont};
		fontMedium == nullptr)
	{
		return std::unexpected(InitError{.stage = "TTF font loading Error", .detail = TTF_GetError()});
	}

	return {};
}

std::expected<void, InitError> SDL_Config::InitTextures()
{
	if (!IMG_Init(IMG_INIT_PNG))
	{
		return std::unexpected(InitError{.stage = "IMG_Init Error", .detail = IMG_GetError()});
	}

	return LoadTexturePair("Images.Logo", logoSurface, logoTexture)
			.and_then([this] { return LoadTexturePair("Images.MenuSelectorP1", selectorIconSurface, selectorIconTexture); })
			.and_then([this] { return LoadPadHints(kPs5Keys, surfacePS5, ps5Textures); })
			.and_then([this] { return LoadPadHints(kXBoxKeys, surfaceXBox, xboxTextures); })
			.and_then([this] { return LoadAtlas(); });
}

std::expected<void, InitError> SDL_Config::InitAudio()
{
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
	{
		return std::unexpected(InitError{.stage = "Mix_OpenAudio Error, sound off", .detail = Mix_GetError()});
	}

	const std::string introMusicPath = PathFromConfig("Music.LevelStarted");
	if (levelIntroMusic = {Mix_LoadWAV(introMusicPath.c_str()), Mix_FreeChunk};
		levelIntroMusic == nullptr)
	{
		return std::unexpected(InitError{.stage = "Mix_LoadWAV Error, sound off", .detail = Mix_GetError()});
	}

	//TODO: move to soundManager
	//NOTE: autoplay only - device and chunk stay ready
	if (gameConfig.skipIntroMusic)
	{
		return {};
	}

	if (Mix_PlayChannel(-1, levelIntroMusic.get(), 0) == -1)
	{
		return std::unexpected(InitError{.stage = "Mix_PlayChannel Error, intro not played",
										 .detail = Mix_GetError()});
	}

	return {};
}

std::string SDL_Config::PathFromConfig(const std::string_view configKey) const
{
	const std::string key{configKey};

	return gameConfig.Get<std::string>(key, key + " path from config.ini");
}

std::expected<std::shared_ptr<SDL_Surface>, InitError> SDL_Config::LoadSurface(const std::string& path)
{
	std::shared_ptr<SDL_Surface> surface{IMG_Load(path.c_str()), SDL_FreeSurface};
	if (surface == nullptr)
	{
		return std::unexpected(InitError{.stage = "IMG " + path + " Loading Error", .detail = IMG_GetError()});
	}

	return surface;
}

std::expected<std::shared_ptr<SDL_Texture>, InitError> SDL_Config::CreateTexture(
		const std::shared_ptr<SDL_Surface>& surface, const std::string& path) const
{
	std::shared_ptr<SDL_Texture> texture{SDL_CreateTextureFromSurface(renderer.get(), surface.get()),
										 SDL_DestroyTexture};
	if (texture == nullptr)
	{
		return std::unexpected(InitError{.stage = "IMG " + path + " Texture Creating Error", .detail = IMG_GetError()});
	}

	return texture;
}

//TODO: split surface loading and texture creation to recreate all texture if vsync change
std::expected<void, InitError> SDL_Config::LoadTexturePair(const std::string_view configKey,
														   std::shared_ptr<SDL_Surface>& outSurface,
														   std::shared_ptr<SDL_Texture>& outTexture)
{
	const std::string path = PathFromConfig(configKey);

	//NOTE: the surface is kept, not dropped after the texture - it is what a vsync change would rebuild from
	return LoadSurface(path).and_then([&](std::shared_ptr<SDL_Surface> surface)
	{
		return CreateTexture(surface, path).transform([&](std::shared_ptr<SDL_Texture> texture)
		{
			outSurface = std::move(surface);
			outTexture = std::move(texture);
		});
	});
}

std::expected<void, InitError> SDL_Config::LoadPadHints(const std::span<const char* const> configKeys,
														std::vector<std::shared_ptr<SDL_Surface>>& outSurfaces,
														std::vector<std::shared_ptr<SDL_Texture>>& outTextures)
{
	outSurfaces.reserve(configKeys.size());
	outTextures.reserve(configKeys.size());

	for (const char* key: configKeys)
	{
		std::shared_ptr<SDL_Surface> surface{nullptr};
		std::shared_ptr<SDL_Texture> texture{nullptr};

		if (auto loaded = LoadTexturePair(key, surface, texture);
			!loaded)
		{
			return loaded;
		}

		outSurfaces.push_back(std::move(surface));
		outTextures.push_back(std::move(texture));
	}

	return {};
}

//NOTE: not LoadTexturePair - the atlas needs its colour key punched into the surface in between
std::expected<void, InitError> SDL_Config::LoadAtlas()
{
	const std::string path = PathFromConfig("Images.SpriteSheet");

	auto surface = LoadSurface(path);
	if (!surface)
	{
		return std::unexpected(surface.error());
	}

	SDL_Surface* rawSurface = surface->get();
	if (SDL_SetColorKey(rawSurface, SDL_TRUE, SDL_MapRGB(rawSurface->format, 0, 0, 1)) != 0)
	{
		return std::unexpected(InitError{.stage = "IMG atlas SetColorKey Error", .detail = SDL_GetError()});
	}

	auto texture = CreateTexture(*surface, path);
	if (!texture)
	{
		return std::unexpected(texture.error());
	}

	SDL_SetTextureBlendMode(texture->get(), SDL_BLENDMODE_BLEND);

	atlasSurface = std::move(*surface);
	atlasTexture = std::move(*texture);

	return {};
}

std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> SDL_Config::InitWindow() const
{
	constexpr auto title = "Battle City remastered";
	constexpr SDL_WindowFlags windowFlags = SDL_WINDOW_RESIZABLE;
	const SDL_Rect rect{.x = static_cast<int>(gameConfig.windowPos.x),
						.y = static_cast<int>(gameConfig.windowPos.y),
						.w = static_cast<int>(gameConfig.windowSize.x),
						.h = static_cast<int>(gameConfig.windowSize.y)};

	return {SDL_CreateWindow(title, rect.x, rect.y, rect.w, rect.h, windowFlags), SDL_DestroyWindow};
}

std::shared_ptr<SDL_Renderer> SDL_Config::InitRender() const
{
	Uint32 renderFlags = SDL_RENDERER_ACCELERATED;
	if (gameConfig.Get<bool>("Window.vsync", false))
	{
		renderFlags |= SDL_RENDERER_PRESENTVSYNC;//TODO: recreate render if vsync change
	}

	const int monitorIndex = gameConfig.Get<int>("Window.MonitorNumber", 1) - 1;
	SDL_Rect bounds;
	SDL_GetDisplayBounds(monitorIndex, &bounds);

	SDL_Rect bordersSize;
	SDL_Window* sdlWindowRaw = sdlWindow.get();
	SDL_GetWindowBordersSize(sdlWindowRaw, &bordersSize.y, &bordersSize.x, &bordersSize.h, &bordersSize.w);

	//NOTE: centering would override an explicit pos
	if (monitorIndex != -1 && !gameConfig.hasExplicitWindowPos)
	{
		const Point screenCenter{.x = bounds.x + bounds.w / 2,
								 .y = bounds.y + bounds.h / 2};
		const Point windowHalfSize{.x = static_cast<int>(gameConfig.windowSize.x) / 2,
								   .y = static_cast<int>(gameConfig.windowSize.y) / 2};
		SDL_SetWindowPosition(sdlWindowRaw,
							  screenCenter.x - windowHalfSize.x + static_cast<int>(gameConfig.windowsPosOffset.x),
							  screenCenter.y - windowHalfSize.y + static_cast<int>(gameConfig.windowsPosOffset.y)
							  - bordersSize.y);
	}

	return {SDL_CreateRenderer(sdlWindowRaw, monitorIndex, renderFlags), SDL_DestroyRenderer};
}
