#pragma once
#include "InitError.h"
#include <filesystem>
#include <SDL.h>//NOTE: do not replace with forward declaration, required for minGW
#include <SDL_mixer.h>
#include <SDL_render.h>
#include <SDL_ttf.h>
#include <expected>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

class GameConfig;
class ProjectConfig;

struct SDL_Config final
{
	static constexpr const char* kWindowTitle{"Battle City remastered"};

	SDL_Config(const GameConfig& config, const ProjectConfig& projectConfig);
	~SDL_Config();

	[[nodiscard]] std::expected<void, InitError> Init();

	void SaveWindowState(ProjectConfig& outProjectConfig) const;

	const GameConfig& gameConfig;
	const ProjectConfig& projectConfig;

	std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> sdlWindow{nullptr, nullptr};
	std::shared_ptr<SDL_Renderer> renderer{nullptr};
	std::shared_ptr<TTF_Font> fontSmall{nullptr};
	std::shared_ptr<TTF_Font> fontMedium{nullptr};
	std::unique_ptr<Mix_Chunk, decltype(&Mix_FreeChunk)> levelIntroMusic{nullptr, nullptr};//TODO: soundManager

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

	[[nodiscard]] std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> InitWindow() const;
	[[nodiscard]] std::shared_ptr<SDL_Renderer> InitRender() const;
};
