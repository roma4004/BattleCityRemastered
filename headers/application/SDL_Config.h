#pragma once
#include "InitError.h"
#include "SdlHandle.h"
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_video.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <expected>
#include <filesystem>
#include <span>
#include <string_view>
#include <vector>

class GameConfig;
class ProjectConfig;
class WindowConfig;

using WindowHandle = SdlHandle<SDL_Window, SDL_DestroyWindow>;
using RendererHandle = SdlHandle<SDL_Renderer, SDL_DestroyRenderer>;
using TextureHandle = SdlHandle<SDL_Texture, SDL_DestroyTexture>;
using SurfaceHandle = SdlHandle<SDL_Surface, SDL_DestroySurface>;
using FontHandle = SdlHandle<TTF_Font, TTF_CloseFont>;
using MixerHandle = SdlHandle<MIX_Mixer, MIX_DestroyMixer>;
using AudioHandle = SdlHandle<MIX_Audio, MIX_DestroyAudio>;

struct SDL_Config final
{
	static constexpr const char* kWindowTitle{"Battle City remastered"};
	static constexpr int kFontSizePtSmall{14};
	static constexpr int kFontSizePtMedium{24};

	[[nodiscard]] FontHandle OpenFont(int pointSize) const;

	SDL_Config(const GameConfig& config, const ProjectConfig& projectConfig, const WindowConfig& windowConfig);
	~SDL_Config();

	[[nodiscard]] std::expected<void, InitError> Init();

	//NOTE: the only way in - the ini value at startup, a runtime switch later
	[[nodiscard]] std::expected<void, InitError> SetVSync(int mode);

	//NOTE: the kept surfaces are the source of truth once a device reset takes the textures
	[[nodiscard]] std::expected<void, InitError> RecreateTexturesFromSurfaces();

	void SaveWindowState(ProjectConfig& outProjectConfig) const;

	const GameConfig& gameConfig;
	const ProjectConfig& projectConfig;
	const WindowConfig& windowConfig;

	WindowHandle sdlWindow{};
	RendererHandle renderer{};
	std::filesystem::path fontPath{};
	//NOTE: opened up front only so a missing font fails Init - drawing goes through the text cache,
	//which opens every size it needs itself
	FontHandle font{};
	//NOTE: the mixer owns the audio device - it has to outlive every MIX_Audio loaded through it
	MixerHandle mixer{};
	AudioHandle levelIntroMusic{};//TODO: soundManager

	TextureHandle logoTexture{};
	TextureHandle atlasTexture{};
	TextureHandle selectorIconTexture{};
	std::vector<TextureHandle> ps5Textures;
	std::vector<TextureHandle> xboxTextures;

	SurfaceHandle logoSurface{};
	SurfaceHandle atlasSurface{};
	SurfaceHandle selectorIconSurface{};
	std::vector<SurfaceHandle> surfacePS5;
	std::vector<SurfaceHandle> surfaceXBox;

private:
	//NOTE: one group per subsystem - each either fills the fields above or names what refused
	[[nodiscard]] std::expected<void, InitError> InitVideo();
	[[nodiscard]] std::expected<void, InitError> InitFonts();
	[[nodiscard]] std::expected<void, InitError> InitTextures();
	//NOTE: can fail like the rest; whether that is fatal is decided in Init(), not here
	[[nodiscard]] std::expected<void, InitError> InitAudio();

	[[nodiscard]] static std::expected<SurfaceHandle, InitError> LoadSurface(
			const std::filesystem::path& path);
	[[nodiscard]] std::expected<TextureHandle, InitError> CreateTexture(
			const SurfaceHandle& surface, const std::filesystem::path& path) const;
	[[nodiscard]] std::expected<void, InitError> LoadTexturePair(std::string_view configKey,
																 SurfaceHandle& outSurface,
																 TextureHandle& outTexture);
	[[nodiscard]] std::expected<void, InitError> LoadPadHints(std::span<const char* const> configKeys,
															  std::vector<SurfaceHandle>& outSurfaces,
															  std::vector<TextureHandle>& outTextures);
	[[nodiscard]] std::expected<void, InitError> LoadAtlas();
	[[nodiscard]] std::expected<void, InitError> RebuildTexture(const SurfaceHandle& surface,
																TextureHandle& outTexture,
																std::string_view name) const;

	//NOTE: an explicit --window-pos/-size is a one-off, and a host/client window is placed by offset -
	//neither belongs in the ini
	[[nodiscard]] bool ShouldPersistWindowPos() const;
	[[nodiscard]] bool ShouldPersistWindowSize() const;

	[[nodiscard]] WindowHandle InitWindow() const;
	[[nodiscard]] RendererHandle InitRender() const;
};
