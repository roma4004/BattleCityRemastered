#pragma once

#include "Point.h"
#include "interfaces/IConfig.h"
#include <SDL.h>
#include <SDL_ttf.h>

class ConfigSuccess final : public IConfig
{
	UPoint _windowSize{};
	std::shared_ptr<SDL_Renderer> _renderer{nullptr};

	//fps
	std::shared_ptr<TTF_Font> _fpsFont{nullptr};

	std::shared_ptr<SDL_Texture> _logoTexture{nullptr};

	std::shared_ptr<SDL_Texture> _atlasTexture{nullptr};

	//user settings //TODO: should be load from config file or default value
	bool _isVsyncOn{false};

public:
	ConfigSuccess() = delete;
	ConfigSuccess(const ConfigSuccess& other) = delete;
	ConfigSuccess(ConfigSuccess&& other) noexcept = delete;

	ConfigSuccess(UPoint windowSize, std::shared_ptr<SDL_Renderer> renderer, std::shared_ptr<TTF_Font> fpsFont,
	              std::shared_ptr<SDL_Texture> logoTexture, std::shared_ptr<SDL_Texture> atlasTexture, bool isVsyncOn);

	~ConfigSuccess() override = default;

	[[nodiscard]] std::unique_ptr<IGame> CreateGame() override;

	ConfigSuccess& operator=(const ConfigSuccess& other) = delete;
	ConfigSuccess& operator=(ConfigSuccess&& other) noexcept = delete;
};
