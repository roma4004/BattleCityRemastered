#pragma once
#include "../headers/Config.h"

#include <SDL.h>
// #include <SDL_ttf.h>

class ConfigSuccess final : public Config
{
	size_t _windowWidth;
	size_t _windowHeight;
	int* _windowBuffer;
	SDL_Renderer* _renderer;
	SDL_Texture* _screen;
	// TTF_Font* font;

public:
	ConfigSuccess() = delete;
	ConfigSuccess(const ConfigSuccess& other) = delete;
	ConfigSuccess(ConfigSuccess&& other) noexcept = delete;

	explicit ConfigSuccess(const size_t windowWidth, const size_t windowHeight, int* windowBuffer,
						   SDL_Renderer* renderer, SDL_Texture* screen /*, TTF_Font* font*/);

	~ConfigSuccess() override = default;

	[[nodiscard]] std::unique_ptr<Game> CreateGame() override;

	ConfigSuccess& operator=(const ConfigSuccess& other) = delete;
	ConfigSuccess& operator=(ConfigSuccess&& other) noexcept = delete;
};
