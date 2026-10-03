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
	static constexpr auto kWindowTitle{"Battle City remastered"};
	static constexpr int kFontSizePtSmall{14};
	static constexpr int kFontSizePtMedium{24};

	[[nodiscard]] FontHandle OpenFont(int pointSize) const;

	SDL_Config(const GameConfig& config, const ProjectConfig& projectConfig, const WindowConfig& windowConfig);
	~SDL_Config();

	[[nodiscard]] std::expected<void, InitError> Init();

	[[nodiscard]] std::expected<void, InitError> SetVSync(int mode) const;

	[[nodiscard]] std::expected<void, InitError> RecreateTexturesFromSurfaces();

	void SaveWindowState(ProjectConfig& outProjectConfig) const;

	const GameConfig& gameConfig;
	const ProjectConfig& projectConfig;
	const WindowConfig& windowConfig;

	WindowHandle sdlWindow{};
	RendererHandle renderer{};
	std::filesystem::path fontPath{};
	//NOTE: opened up front so a missing font fails Init; the text cache opens its own sizes, this one is its fallback
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
	[[nodiscard]] std::expected<void, InitError> InitVideo();
	[[nodiscard]] std::expected<void, InitError> InitFonts();
	[[nodiscard]] std::expected<void, InitError> InitTextures();
	[[nodiscard]] std::expected<void, InitError> InitAudio();

	[[nodiscard]] std::expected<TextureHandle, InitError> CreateTexture(
			const SurfaceHandle& surface, const std::filesystem::path& path) const;
	[[nodiscard]] std::expected<void, InitError> LoadTexturePair(std::string_view configKey,
																 SurfaceHandle& outSurface,
																 TextureHandle& outTexture) const;
	[[nodiscard]] std::expected<void, InitError> LoadPadHints(std::span<const char* const> configKeys,
															  std::vector<SurfaceHandle>& outSurfaces,
															  std::vector<TextureHandle>& outTextures);
	[[nodiscard]] std::expected<void, InitError> LoadAtlas();
	[[nodiscard]] std::expected<void, InitError> RebuildTexture(const SurfaceHandle& surface,
																TextureHandle& outTexture,
																std::string_view name) const;

	//NOTE: a pos/size from the command line and a window put on a side are one-offs - neither goes to the ini
	[[nodiscard]] bool ShouldPersistWindowPos() const;
	[[nodiscard]] bool ShouldPersistWindowSize() const;

	[[nodiscard]] WindowHandle InitWindow() const;
	[[nodiscard]] RendererHandle InitRender() const;
};
