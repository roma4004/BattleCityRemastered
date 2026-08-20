#include "application/SDL_Config.h"
#include "application/ProjectConfig.h"
#include "utils/Log.h"
#include "application/GameConfig.h"
#include "application/UserInput.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <algorithm>
#include <array>
#include <memory>

namespace
{
//NOTE: positional - RenderManager draws these vectors by index, so the order is part of the contract
constexpr std::array kPs5Keys{"Images.PS5_Create",
							  "Images.PS5_Cross",
							  "Images.PS5_D-Pad",
							  "Images.PS5_Home",
							  "Images.PS5_Options",
							  "Images.PS5_Triangle"};
constexpr std::array kXBoxKeys{"Images.XBox_D-Pad",
							   "Images.XBox_Home",
							   "Images.XBox_Menu",
							   "Images.XBox_View",
							   "Images.XBox_A",
							   "Images.XBox_Y"};
}

SDL_Config::SDL_Config(GameConfig& config, const ProjectConfig& project)
	: gameConfig{config}
	, projectConfig{project} {}

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
				   Log::Error(audio.error().stage + ": " + audio.error().detail);
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

	const std::string fontPath = projectConfig.ResourcePath("Fonts.BattleCity").string();

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
		  .and_then([this]
		   {
			   return LoadTexturePair("Images.MenuSelectorP1", selectorIconSurface, selectorIconTexture);
		   })
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

	const std::string introMusicPath = projectConfig.ResourcePath("Music.LevelStarted").string();
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

std::expected<std::shared_ptr<SDL_Surface>, InitError> SDL_Config::LoadSurface(const std::filesystem::path& path)
{
	std::shared_ptr<SDL_Surface> surface{IMG_Load(path.string().c_str()), SDL_FreeSurface};
	if (surface == nullptr)
	{
		return std::unexpected(
				InitError{.stage = "IMG " + path.string() + " Loading Error", .detail = IMG_GetError()});
	}

	return surface;
}

std::expected<std::shared_ptr<SDL_Texture>, InitError> SDL_Config::CreateTexture(
		const std::shared_ptr<SDL_Surface>& surface, const std::filesystem::path& path) const
{
	std::shared_ptr<SDL_Texture> texture{SDL_CreateTextureFromSurface(renderer.get(), surface.get()),
										 SDL_DestroyTexture};
	if (texture == nullptr)
	{
		return std::unexpected(
				InitError{.stage = "IMG " + path.string() + " Texture Creating Error", .detail = IMG_GetError()});
	}

	return texture;
}

//TODO: split surface loading and texture creation to recreate all texture if vsync change
std::expected<void, InitError> SDL_Config::LoadTexturePair(const std::string_view configKey,
														   std::shared_ptr<SDL_Surface>& outSurface,
														   std::shared_ptr<SDL_Texture>& outTexture)
{
	const std::filesystem::path path = projectConfig.ResourcePath(std::string{configKey});

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
	const std::filesystem::path path = projectConfig.ResourcePath("Images.SpriteSheet");

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

void SDL_Config::SaveWindowState(ProjectConfig& outProjectConfig) const
{
	SDL_Window* sdlWindowRaw = sdlWindow.get();
	if (sdlWindowRaw == nullptr)
	{
		return;
	}

	constexpr Uint32 unsavableFlags = SDL_WINDOW_MINIMIZED | SDL_WINDOW_MAXIMIZED;
	if ((SDL_GetWindowFlags(sdlWindowRaw) & unsavableFlags) != 0u)
	{
		return;
	}

	if (gameConfig.ShouldPersistWindowPos())
	{
		int posX{};
		int posY{};
		SDL_GetWindowPosition(sdlWindowRaw, &posX, &posY);

		int width{};
		int height{};
		SDL_GetWindowSize(sdlWindowRaw, &width, &height);

		SDL_Rect bounds{};
		if (SDL_GetDisplayUsableBounds(SDL_GetWindowDisplayIndex(sdlWindowRaw), &bounds) != 0)
		{
			bounds = SDL_Rect{.x = 0, .y = 0, .w = width, .h = height};
		}

		int topBorder{};
		SDL_GetWindowBordersSize(sdlWindowRaw, &topBorder, nullptr, nullptr, nullptr);

		const int minX = std::max(0, bounds.x);
		const int minY = std::max(0, bounds.y + topBorder);
		const int maxX = std::max(minX, bounds.x + bounds.w - width);
		const int maxY = std::max(minY, bounds.y + bounds.h - height);

		outProjectConfig.Set("Window.posX", static_cast<unsigned>(std::clamp(posX, minX, maxX)));
		outProjectConfig.Set("Window.posY", static_cast<unsigned>(std::clamp(posY, minY, maxY)));
	}

	if (gameConfig.ShouldPersistWindowSize())
	{
		int width{};
		int height{};
		SDL_GetWindowSize(sdlWindowRaw, &width, &height);

		outProjectConfig.Set("Window.width", static_cast<unsigned>(std::max(0, width)));
		outProjectConfig.Set("Window.height", static_cast<unsigned>(std::max(0, height)));
	}
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
	if (projectConfig.IsVsyncOn())
	{
		renderFlags |= SDL_RENDERER_PRESENTVSYNC;//TODO: recreate render if vsync change
	}

	const int monitorIndex = projectConfig.MonitorNumber() - 1;
	SDL_Rect bounds;
	SDL_GetDisplayBounds(monitorIndex, &bounds);

	SDL_Rect bordersSize;
	SDL_Window* sdlWindowRaw = sdlWindow.get();
	SDL_GetWindowBordersSize(sdlWindowRaw, &bordersSize.y, &bordersSize.x, &bordersSize.h, &bordersSize.w);

	//NOTE: centering would override an explicit pos
	const bool centerOnMonitor =
			!gameConfig.hasExplicitWindowPos
			&& (projectConfig.IsFreshIni()
				|| projectConfig.IsCenterOnStart()
				|| gameConfig.IsHost()
				|| gameConfig.IsClient());
	if (monitorIndex != -1 && centerOnMonitor)
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
