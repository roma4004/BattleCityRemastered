#pragma once
#include <SDL.h>//NOTE: do not replace with forward declaration, required for minGW
#include <SDL_mixer.h>
#include <SDL_render.h>
#include <SDL_ttf.h>
#include <memory>
#include <vector>

class IConfig;
class GameConfig;

struct SDL_Config final
{
	explicit SDL_Config(GameConfig& gameConfig);
	~SDL_Config();

	[[nodiscard]] std::unique_ptr<IConfig> Init();

	GameConfig& gameConfig;

	std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> sdlWindow{nullptr, nullptr};
	std::shared_ptr<SDL_Renderer> renderer{nullptr};
	std::shared_ptr<TTF_Font> fontSmall{nullptr};
	std::shared_ptr<TTF_Font> fontMedium{nullptr};
	std::unique_ptr<Mix_Chunk, decltype(&Mix_FreeChunk)> levelIntroMusic{nullptr, nullptr}; //TODO: soundManager

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
	[[nodiscard]] std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> InitWindow() const;
	[[nodiscard]] std::shared_ptr<SDL_Renderer> InitRender() const;
};
