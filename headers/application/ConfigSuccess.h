#pragma once

#include "../Point.h"
#include "../interfaces/IConfig.h"

#include <SDL.h>
#include <SDL_ttf.h>

class ConfigSuccess final : public IConfig
{
	UPoint _windowSize{.x = 0, .y = 0};
	std::shared_ptr<int[]> _windowBuffer{nullptr};
	std::shared_ptr<SDL_Renderer> _renderer{nullptr};
	std::shared_ptr<SDL_Texture> _screen{nullptr};

	//fps
	std::shared_ptr<TTF_Font> _fpsFont{nullptr};

	std::shared_ptr<SDL_Texture> _logoTexture{nullptr};

public:
	ConfigSuccess() = delete;
	ConfigSuccess(const ConfigSuccess& other) = delete;
	ConfigSuccess(ConfigSuccess&& other) noexcept = delete;

	ConfigSuccess(UPoint windowSize, std::shared_ptr<int[]> windowBuffer, std::shared_ptr<SDL_Renderer> renderer,
	              std::shared_ptr<SDL_Texture> screen, std::shared_ptr<TTF_Font> fpsFont,
	              std::shared_ptr<SDL_Texture> logoTexture);

	~ConfigSuccess() override = default;

	[[nodiscard]] std::unique_ptr<IGame> CreateGame() override;

	ConfigSuccess& operator=(const ConfigSuccess& other) = delete;
	ConfigSuccess& operator=(ConfigSuccess&& other) noexcept = delete;
};
