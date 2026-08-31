#include "application/SDL_Config.h"
#include "application/ProjectConfig.h"
#include "utils/Log.h"
#include "application/GameConfig.h"
#include "application/WindowConfig.h"
#include "application/UserInput.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
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

SDL_Config::SDL_Config(const GameConfig& config, const ProjectConfig& project, const WindowConfig& window)
	: gameConfig{config}
	, projectConfig{project}
	, windowConfig{window} {}

bool SDL_Config::ShouldPersistWindowPos() const
{
	return !windowConfig.hasExplicitPos && !gameConfig.IsHost() && !gameConfig.IsClient();
}

bool SDL_Config::ShouldPersistWindowSize() const
{
	return !windowConfig.hasExplicitSize && !gameConfig.IsHost() && !gameConfig.IsClient();
}

SDL_Config::~SDL_Config()
{
	levelIntroMusic.reset();
	mixer.reset();
	MIX_Quit();
	font.reset();
	TTF_Quit();
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
	//NOTE: SDL3 dropped SDL_INIT_EVERYTHING; EVENTS comes in with VIDEO, JOYSTICK with GAMEPAD
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD))
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

	//NOTE: SDL3 flipped the default to linear filtering; on pixel art that blurs every sprite and, on a
	//shared atlas, bleeds neighbouring cells into each other at the edges
	SDL_SetDefaultTextureScaleMode(renderer.get(), SDL_SCALEMODE_NEAREST);

	if (const auto vsync = SetVSync(projectConfig.VSyncMode());
		!vsync)
	{
		return std::unexpected(vsync.error());
	}

	SDL_ShowWindow(sdlWindow.get());

	return {};
}

//TODO: runtime switch - update Window.vsync in ProjectConfig too, FramePerSecondManager reads it
//every frame; the renderer and its textures survive the call
std::expected<void, InitError> SDL_Config::SetVSync(const int mode)
{
	if (!SDL_SetRenderVSync(renderer.get(), mode))
	{
		return std::unexpected(InitError{.stage = "SDL_SetRenderVSync Error", .detail = SDL_GetError()});
	}

	return {};
}

std::expected<void, InitError> SDL_Config::InitFonts()
{
	if (!TTF_Init())
	{
		return std::unexpected(InitError{.stage = "TTF_Init Error", .detail = SDL_GetError()});
	}

	fontPath = projectConfig.ResourcePath("Fonts.BattleCity");

	if (font = OpenFont(kFontSizePtMedium);
		font == nullptr)
	{
		return std::unexpected(InitError{.stage = "TTF font loading Error", .detail = SDL_GetError()});
	}

	return {};
}

FontHandle SDL_Config::OpenFont(const int pointSize) const
{
	return FontHandle{TTF_OpenFont(fontPath.string().c_str(), static_cast<float>(pointSize))};
}

std::expected<void, InitError> SDL_Config::InitTextures()
{
	//NOTE: SDL3_image dropped IMG_Init/IMG_Quit - the decoders are always in
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
	if (!MIX_Init())
	{
		return std::unexpected(InitError{.stage = "MIX_Init Error, sound off", .detail = SDL_GetError()});
	}

	//NOTE: SDL3_mixer traded the one global device for an explicit mixer - a null spec lets it pick
	if (mixer = MixerHandle{MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr)};
		mixer == nullptr)
	{
		return std::unexpected(InitError{.stage = "MIX_CreateMixerDevice Error, sound off",
										 .detail = SDL_GetError()});
	}

	const std::string introMusicPath = projectConfig.ResourcePath("Music.LevelStarted").string();
	//NOTE: predecoded - one short chunk, and decoding it once keeps the playback path allocation-free
	if (levelIntroMusic = AudioHandle{MIX_LoadAudio(mixer.get(), introMusicPath.c_str(), true)};
		levelIntroMusic == nullptr)
	{
		return std::unexpected(InitError{.stage = "MIX_LoadAudio Error, sound off", .detail = SDL_GetError()});
	}

	//TODO: move to soundManager
	//NOTE: autoplay only - device and chunk stay ready
	if (gameConfig.skipIntroMusic)
	{
		return {};
	}

	if (!MIX_PlayAudio(mixer.get(), levelIntroMusic.get()))
	{
		return std::unexpected(InitError{.stage = "MIX_PlayAudio Error, intro not played",
										 .detail = SDL_GetError()});
	}

	return {};
}

std::expected<SurfaceHandle, InitError> SDL_Config::LoadSurface(const std::filesystem::path& path)
{
	SurfaceHandle surface{IMG_Load(path.string().c_str())};
	if (surface == nullptr)
	{
		return std::unexpected(
				InitError{.stage = "IMG " + path.string() + " Loading Error", .detail = SDL_GetError()});
	}

	return surface;
}

std::expected<TextureHandle, InitError> SDL_Config::CreateTexture(
		const SurfaceHandle& surface, const std::filesystem::path& path) const
{
	TextureHandle texture{SDL_CreateTextureFromSurface(renderer.get(), surface.get())};
	if (texture == nullptr)
	{
		return std::unexpected(
				InitError{.stage = "IMG " + path.string() + " Texture Creating Error", .detail = SDL_GetError()});
	}

	return texture;
}

std::expected<void, InitError> SDL_Config::LoadTexturePair(const std::string_view configKey,
														   SurfaceHandle& outSurface,
														   TextureHandle& outTexture)
{
	const std::filesystem::path path = projectConfig.ResourcePath(std::string{configKey});

	//NOTE: the surface is kept - a device reset rebuilds the texture from it
	return LoadSurface(path).and_then([&](SurfaceHandle surface)
	{
		return CreateTexture(surface, path).transform([&](TextureHandle texture)
		{
			outSurface = std::move(surface);
			outTexture = std::move(texture);
		});
	});
}

std::expected<void, InitError> SDL_Config::LoadPadHints(const std::span<const char* const> configKeys,
														std::vector<SurfaceHandle>& outSurfaces,
														std::vector<TextureHandle>& outTextures)
{
	outSurfaces.reserve(configKeys.size());
	outTextures.reserve(configKeys.size());

	for (const char* key: configKeys)
	{
		SurfaceHandle surface{nullptr};
		TextureHandle texture{nullptr};

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
	//NOTE: SDL3 keeps only the format enum on the surface - the channel layout comes from its details
	const SDL_PixelFormatDetails* formatDetails = SDL_GetPixelFormatDetails(rawSurface->format);
	const Uint32 colorKey = SDL_MapRGB(formatDetails, SDL_GetSurfacePalette(rawSurface), 0, 0, 1);
	if (!SDL_SetSurfaceColorKey(rawSurface, true, colorKey))
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

std::expected<void, InitError> SDL_Config::RebuildTexture(const SurfaceHandle& surface,
														  TextureHandle& outTexture,
														  const std::string_view name) const
{
	if (surface == nullptr)
	{
		return {};
	}

	return CreateTexture(surface, name).transform([&outTexture](TextureHandle texture)
	{
		outTexture = std::move(texture);
	});
}

std::expected<void, InitError> SDL_Config::RecreateTexturesFromSurfaces()
{
	const auto rebuildPadHints = [this](const std::vector<SurfaceHandle>& surfaces,
										std::vector<TextureHandle>& outTextures,
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

	constexpr SDL_WindowFlags unsavableFlags = SDL_WINDOW_MINIMIZED | SDL_WINDOW_MAXIMIZED;
	if ((SDL_GetWindowFlags(sdlWindowRaw) & unsavableFlags) != 0u)
	{
		return;
	}

	if (ShouldPersistWindowPos())
	{
		int posX{};
		int posY{};
		SDL_GetWindowPosition(sdlWindowRaw, &posX, &posY);

		int width{};
		int height{};
		SDL_GetWindowSize(sdlWindowRaw, &width, &height);

		SDL_Rect bounds{};
		if (!SDL_GetDisplayUsableBounds(SDL_GetDisplayForWindow(sdlWindowRaw), &bounds))
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

	if (ShouldPersistWindowSize())
	{
		int width{};
		int height{};
		SDL_GetWindowSize(sdlWindowRaw, &width, &height);

		//NOTE: stored unscaled, as InitWindow reads it. Queried live - the window may have moved to a
		//display with another scale
		const float scale = SDL_GetWindowDisplayScale(sdlWindowRaw);
		const double divisor = scale > 0.0f ? static_cast<double>(scale) : 1.0;

		outProjectConfig.Set("Window.width",
							 static_cast<unsigned>(std::max(0L, std::lround(static_cast<double>(width) / divisor))));
		outProjectConfig.Set("Window.height",
							 static_cast<unsigned>(std::max(0L, std::lround(static_cast<double>(height) / divisor))));
	}
}

WindowHandle SDL_Config::InitWindow() const
{
	//NOTE: hidden until InitVideo is through - it is resized and moved right after creation
	constexpr SDL_WindowFlags windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN;

	WindowHandle window{SDL_CreateWindow(kWindowTitle,
										static_cast<int>(windowConfig.size.x),
										static_cast<int>(windowConfig.size.y),
										windowFlags)};

	if (window == nullptr)
	{
		return window;
	}

	//NOTE: SDL3 window coordinates are plain pixels - the process is DPI aware and the desktop scale is
	//not folded in, so the stored size is read as unscaled and multiplied here. SaveWindowState divides
	//it back out, else the window would grow by the scale every run.
	if (const float scale = SDL_GetWindowDisplayScale(window.get());
		scale > 0.0f)
	{
		const double factor = static_cast<double>(scale);
		SDL_SetWindowSize(window.get(),
						  static_cast<int>(std::lround(static_cast<double>(windowConfig.size.x) * factor)),
						  static_cast<int>(std::lround(static_cast<double>(windowConfig.size.y) * factor)));
	}

	//NOTE: SDL3 dropped the position from SDL_CreateWindow - it opens centred and is moved afterwards
	SDL_SetWindowPosition(window.get(),
						  static_cast<int>(windowConfig.pos.x),
						  static_cast<int>(windowConfig.pos.y));

	return window;
}

RendererHandle SDL_Config::InitRender() const
{
	const int monitorIndex = projectConfig.MonitorNumber() - 1;

	//NOTE: SDL3 addresses displays by id, not by index - the ini still holds the 1-based number
	SDL_Rect bounds{};
	SDL_Rect usableBounds{};
	bool hasMonitor{false};
	if (int displayCount{}; SDL_DisplayID* displays = SDL_GetDisplays(&displayCount))
	{
		hasMonitor = monitorIndex >= 0
					 && monitorIndex < displayCount
					 && SDL_GetDisplayBounds(displays[monitorIndex], &bounds);
		//NOTE: usable leaves the taskbar out - centring goes by the full bounds, the clamp by these
		if (hasMonitor && !SDL_GetDisplayUsableBounds(displays[monitorIndex], &usableBounds))
		{
			usableBounds = bounds;
		}
		SDL_free(displays);
	}

	SDL_Rect bordersSize{};
	SDL_Window* sdlWindowRaw = sdlWindow.get();
	SDL_GetWindowBordersSize(sdlWindowRaw, &bordersSize.y, &bordersSize.x, &bordersSize.h, &bordersSize.w);

	//NOTE: centering would override an explicit pos
	const bool centerOnMonitor =
			!windowConfig.hasExplicitPos
			&& (projectConfig.IsFreshIni()
				|| projectConfig.IsCenterOnStart()
				|| gameConfig.IsHost()
				|| gameConfig.IsClient());
	if (hasMonitor && centerOnMonitor)
	{
		//NOTE: the real window, not the ini one - InitWindow already scaled it
		int windowWidth{};
		int windowHeight{};
		SDL_GetWindowSize(sdlWindowRaw, &windowWidth, &windowHeight);

		//NOTE: WindowConfig gives the direction, the distance comes off the real window - already
		//scaled. The cast to int unwraps the host's negative offset, made by an unsigned subtraction.
		const auto halfWindowApart = [](const std::size_t offset, const int windowSide)
		{
			const int direction = static_cast<int>(0 < static_cast<int>(offset))
								  - static_cast<int>(static_cast<int>(offset) < 0);

			return direction * (windowSide / 2);
		};

		const Point screenCenter{.x = bounds.x + bounds.w / 2,
								 .y = bounds.y + bounds.h / 2};
		const Point centred{.x = screenCenter.x - windowWidth / 2
								 + halfWindowApart(windowConfig.posOffset.x, windowWidth),
							.y = screenCenter.y - windowHeight / 2
								 + halfWindowApart(windowConfig.posOffset.y, windowHeight)
								 - bordersSize.y};

		//NOTE: the pair spans two windows and must stay on this display. Clamped, not shrunk - they
		//overlap in the middle instead of leaving the screen; the top margin keeps the title bar
		const int minX = usableBounds.x;
		const int minY = usableBounds.y + bordersSize.y;
		const int maxX = std::max(minX, usableBounds.x + usableBounds.w - windowWidth);
		const int maxY = std::max(minY, usableBounds.y + usableBounds.h - windowHeight);

		SDL_SetWindowPosition(sdlWindowRaw, std::clamp(centred.x, minX, maxX), std::clamp(centred.y, minY, maxY));
	}

	//NOTE: vsync is not a creation flag - InitVideo applies it through SetVSync
	return RendererHandle{SDL_CreateRenderer(sdlWindowRaw, nullptr)};
}
