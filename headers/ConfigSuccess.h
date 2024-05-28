#pragma once
#include "../headers/Config.h"

#include <SDL.h>
// #include <SDL_ttf.h>

class ConfigSuccess final : public Config
{
	UPoint _windowSize{0, 0};
	int* _windowBuffer{nullptr};
	SDL_Renderer* _renderer{nullptr};
	SDL_Texture* _screen{nullptr};
	// TTF_Font* font;

public:
	ConfigSuccess() = delete;
	ConfigSuccess(const ConfigSuccess& other) = delete;
	ConfigSuccess(ConfigSuccess&& other) noexcept = delete;

	explicit ConfigSuccess(UPoint windowSize, int* windowBuffer, SDL_Renderer* renderer,
						   SDL_Texture* screen /*, TTF_Font* font*/);

	~ConfigSuccess() override = default;

	[[nodiscard]] std::unique_ptr<Game> CreateGame() override;

	ConfigSuccess& operator=(const ConfigSuccess& other) = delete;
	ConfigSuccess& operator=(ConfigSuccess&& other) noexcept = delete;
};
