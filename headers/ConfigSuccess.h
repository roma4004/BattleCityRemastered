#pragma once

#include "IConfig.h"
#include "Point.h"

#include <SDL.h>
#include <SDL_ttf.h>

class ConfigSuccess final : public IConfig
{
	UPoint _windowSize{0, 0};
	int* _windowBuffer{nullptr};
	SDL_Renderer* _renderer{nullptr};
	SDL_Texture* _screen{nullptr};

	//fps
	TTF_Font* _fpsFont{nullptr};
	// SDL_Surface* fpsSurface{nullptr};
	// SDL_Texture* fpsTexture{nullptr};

public:
	ConfigSuccess() = delete;
	ConfigSuccess(const ConfigSuccess& other) = delete;
	ConfigSuccess(ConfigSuccess&& other) noexcept = delete;

	explicit ConfigSuccess(UPoint windowSize, int* windowBuffer, SDL_Renderer* renderer, SDL_Texture* screen,
	                       TTF_Font* fpsFont);

	~ConfigSuccess() override = default;

	[[nodiscard]] std::unique_ptr<IGame> CreateGame() override;

	ConfigSuccess& operator=(const ConfigSuccess& other) = delete;
	ConfigSuccess& operator=(ConfigSuccess&& other) noexcept = delete;
};
