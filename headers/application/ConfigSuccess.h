#pragma once

#include "Point.h"
#include "interfaces/IConfig.h"
#include <SDL.h>
#include <SDL_ttf.h>

class ConfigSuccess final : public IConfig
{
	UPoint _windowSize{};

	std::shared_ptr<SDL_Renderer> _renderer{nullptr};
	std::shared_ptr<TTF_Font> _fpsFont{nullptr};
	std::shared_ptr<SDL_Texture> _logoTexture{nullptr};
	std::shared_ptr<SDL_Texture> _atlasTexture{nullptr};
	std::shared_ptr<SDL_Texture> _joyIcon{nullptr};
	std::shared_ptr<SDL_Texture> _p1ControlHint{nullptr};
	std::shared_ptr<SDL_Texture> _p2ControlHint{nullptr};

	//user settings
	bool _isVsyncOn{};//TODO: should be load from config file or default value

public:
	ConfigSuccess() = delete;
	ConfigSuccess(const ConfigSuccess& other) = delete;
	ConfigSuccess(ConfigSuccess&& other) noexcept = delete;

	ConfigSuccess(UPoint windowSize, const std::shared_ptr<SDL_Renderer>& renderer,
				  const std::shared_ptr<TTF_Font>& fpsFont, const std::shared_ptr<SDL_Texture>& logoTexture,
				  const std::shared_ptr<SDL_Texture>& atlasTexture, const std::shared_ptr<SDL_Texture>& joyIcon,
				  const std::shared_ptr<SDL_Texture>& p1ControlHint, const std::shared_ptr<SDL_Texture>& p2ControlHint,
				  bool isVsyncOn);

	~ConfigSuccess() override = default;

	[[nodiscard]] std::unique_ptr<IGame> CreateGame() override;

	ConfigSuccess& operator=(const ConfigSuccess& other) = delete;
	ConfigSuccess& operator=(ConfigSuccess&& other) noexcept = delete;
};
