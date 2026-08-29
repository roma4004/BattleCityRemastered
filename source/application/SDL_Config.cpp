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

SDL_Config::SDL_Config(const GameConfig& config, const ProjectConfig& project)
	: gameConfig{config}
	, projectConfig{project} {}

SDL_Config::~SDL_Config()
{
	Mix_CloseAudio();
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

	return SetVSync(projectConfig.IsVsyncOn());
}

//TODO: runtime switch - update Window.vsync in ProjectConfig too, FramePerSecondManager reads it
//every frame; the renderer and its textures survive the call
std::expected<void, InitError> SDL_Config::SetVSync(const bool isOn)
{
	if (SDL_RenderSetVSync(renderer.get(), isOn ? 1 : 0) != 0)
	{
		return std::unexpected(InitError{.stage = "SDL_RenderSetVSync Error", .detail = SDL_GetError()});
	}

	return {};
}

std::expected<void, InitError> SDL_Config::InitFonts()
{
	if (TTF_Init() == -1)
	{
		return std::unexpected(InitError{.stage = "TTF_Init Error", .detail = TTF_GetError()});
	}

	fontPath = projectConfig.ResourcePath("Fonts.BattleCity");

	if (fontMedium = OpenFont(kFontSizePtMedium);
		fontMedium == nullptr)
	{
		return std::unexpected(InitError{.stage = "TTF font loading Error", .detail = TTF_GetError()});
	}

	return {};
}

std::shared_ptr<TTF_Font> SDL_Config::OpenFont(const int pointSize) const
{
	return {TTF_OpenFont(fontPath.string().c_str(), pointSize), TTF_CloseFont};
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

std::expected<void, InitError> SDL_Config::LoadTexturePair(const std::string_view configKey,
														   std::shared_ptr<SDL_Surface>& outSurface,
														   std::shared_ptr<SDL_Texture>& outTexture)
{
	const std::filesystem::path path = projectConfig.ResourcePath(std::string{configKey});

	//NOTE: the surface is kept - a device reset rebuilds the texture from it
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

std::expected<void, InitError> SDL_Config::RebuildTexture(const std::shared_ptr<SDL_Surface>& surface,
														  std::shared_ptr<SDL_Texture>& outTexture,
														  const std::string_view name) const
{
	if (surface == nullptr)
	{
		return {};
	}

	return CreateTexture(surface, name).transform([&outTexture](std::shared_ptr<SDL_Texture> texture)
	{
		outTexture = std::move(texture);
	});
}

std::expected<void, InitError> SDL_Config::RecreateTexturesFromSurfaces()
{
	const auto rebuildPadHints = [this](const std::vector<std::shared_ptr<SDL_Surface>>& surfaces,
										std::vector<std::shared_ptr<SDL_Texture>>& outTextures,
										const std::string_view name) -> std::expected<void, InitError>
	{
		outTextures.resize(surfaces.size());

		for (size_t i = 0u; i < surfaces.size(); ++i)
		{
			if (auto rebuilt = RebuildTexture(surfaces[i], outTextures[i], name);
				!rebuilt)
			{
				return rebuilt;
			}
		}

		return {};
	};

	return RebuildTexture(logoSurface, logoTexture, "Images.Logo")
		  .and_then([this] { return RebuildTexture(selectorIconSurface, selectorIconTexture, "Images.MenuSelectorP1"); })
		  .and_then([&rebuildPadHints, this] { return rebuildPadHints(surfacePS5, ps5Textures, "Images.PS5"); })
		  .and_then([&rebuildPadHints, this] { return rebuildPadHints(surfaceXBox, xboxTextures, "Images.XBox"); })
		  .and_then([this]
		   {
			   //NOTE: the color key rides in the surface, the blend mode does not - set it again
			   return RebuildTexture(atlasSurface, atlasTexture, "Images.SpriteSheet")
					   .transform([this] { SDL_SetTextureBlendMode(atlasTexture.get(), SDL_BLENDMODE_BLEND); });
		   });
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
	constexpr SDL_WindowFlags windowFlags = SDL_WINDOW_RESIZABLE;
	const SDL_Rect rect{.x = static_cast<int>(gameConfig.windowPos.x),
						.y = static_cast<int>(gameConfig.windowPos.y),
						.w = static_cast<int>(gameConfig.windowSize.x),
						.h = static_cast<int>(gameConfig.windowSize.y)};

	return {SDL_CreateWindow(kWindowTitle, rect.x, rect.y, rect.w, rect.h, windowFlags), SDL_DestroyWindow};
}

std::shared_ptr<SDL_Renderer> SDL_Config::InitRender() const
{
	//NOTE: vsync is not a creation flag - InitVideo applies it through SetVSync
	constexpr Uint32 renderFlags = SDL_RENDERER_ACCELERATED;

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
