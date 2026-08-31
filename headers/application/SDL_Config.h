#pragma once
#include "InitError.h"
#include <filesystem>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>//NOTE: decltype(&SDL_DestroyWindow) needs the function declared, not just the type
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <expected>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

class GameConfig;
class ProjectConfig;
class WindowConfig;

struct SDL_Config final
{
	static constexpr const char* kWindowTitle{"Battle City remastered"};
	static constexpr int kFontSizePtSmall{14};
	static constexpr int kFontSizePtMedium{24};

	[[nodiscard]] std::shared_ptr<TTF_Font> OpenFont(int pointSize) const;

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

	std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> sdlWindow{nullptr, nullptr};
	std::shared_ptr<SDL_Renderer> renderer{nullptr};
	std::filesystem::path fontPath{};
	//NOTE: opened up front only so a missing font fails Init - drawing goes through the text cache,
	//which opens every size it needs itself
	std::shared_ptr<TTF_Font> font{nullptr};
	//NOTE: the mixer owns the audio device - it has to outlive every MIX_Audio loaded through it
	std::unique_ptr<MIX_Mixer, decltype(&MIX_DestroyMixer)> mixer{nullptr, nullptr};
	std::unique_ptr<MIX_Audio, decltype(&MIX_DestroyAudio)> levelIntroMusic{nullptr, nullptr};//TODO: soundManager

	std::shared_ptr<SDL_Texture> logoTexture{nullptr};
	std::shared_ptr<SDL_Texture> atlasTexture{nullptr};
	std::shared_ptr<SDL_Texture> selectorIconTexture{nullptr};
	std::vector<std::shared_ptr<SDL_Texture>> ps5Textures;
	std::vector<std::shared_ptr<SDL_Texture>> xboxTextures;

	std::shared_ptr<SDL_Surface> logoSurface{nullptr};
	std::shared_ptr<SDL_Surface> atlasSurface{nullptr};
	std::shared_ptr<SDL_Surface> selectorIconSurface{nullptr};
	std::vector<std::shared_ptr<SDL_Surface>> surfacePS5;
	std::vector<std::shared_ptr<SDL_Surface>> surfaceXBox;

private:
	//NOTE: one group per subsystem - each either fills the fields above or names what refused
	[[nodiscard]] std::expected<void, InitError> InitVideo();
	[[nodiscard]] std::expected<void, InitError> InitFonts();
	[[nodiscard]] std::expected<void, InitError> InitTextures();
	//NOTE: can fail like the rest; whether that is fatal is decided in Init(), not here
	[[nodiscard]] std::expected<void, InitError> InitAudio();

	[[nodiscard]] static std::expected<std::shared_ptr<SDL_Surface>, InitError> LoadSurface(
			const std::filesystem::path& path);
	[[nodiscard]] std::expected<std::shared_ptr<SDL_Texture>, InitError> CreateTexture(
			const std::shared_ptr<SDL_Surface>& surface, const std::filesystem::path& path) const;
	[[nodiscard]] std::expected<void, InitError> LoadTexturePair(std::string_view configKey,
																 std::shared_ptr<SDL_Surface>& outSurface,
																 std::shared_ptr<SDL_Texture>& outTexture);
	[[nodiscard]] std::expected<void, InitError> LoadPadHints(std::span<const char* const> configKeys,
															  std::vector<std::shared_ptr<SDL_Surface>>& outSurfaces,
															  std::vector<std::shared_ptr<SDL_Texture>>& outTextures);
	[[nodiscard]] std::expected<void, InitError> LoadAtlas();
	[[nodiscard]] std::expected<void, InitError> RebuildTexture(const std::shared_ptr<SDL_Surface>& surface,
																std::shared_ptr<SDL_Texture>& outTexture,
																std::string_view name) const;

	//NOTE: an explicit --window-pos/-size is a one-off, and a host/client window is placed by offset -
	//neither belongs in the ini
	[[nodiscard]] bool ShouldPersistWindowPos() const;
	[[nodiscard]] bool ShouldPersistWindowSize() const;

	[[nodiscard]] std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> InitWindow() const;
	[[nodiscard]] std::shared_ptr<SDL_Renderer> InitRender() const;
};
