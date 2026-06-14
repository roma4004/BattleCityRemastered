#pragma once

#include "Point.h"
#include "interfaces/IConfig.h"
#include <SDL.h>
#include <SDL_ttf.h>

class ConfigSuccess final : public IConfig
{
	UPoint _windowSize{};

	std::shared_ptr<SDL_Renderer> _renderer{nullptr};
	std::shared_ptr<TTF_Font> _font{nullptr};
	std::shared_ptr<SDL_Texture> _logo{nullptr};
	std::shared_ptr<SDL_Texture> _atlas{nullptr};
	std::shared_ptr<SDL_Texture> _joyIcon{nullptr};
	std::shared_ptr<SDL_Texture> _xBoxHint{nullptr};
	std::shared_ptr<SDL_Texture> _pS5Hint{nullptr};

	//user settings
	bool _isVsyncOn{};//TODO: should be load from config file or default value

public:
	ConfigSuccess() = delete;
	ConfigSuccess(const ConfigSuccess& other) = delete;
	ConfigSuccess(ConfigSuccess&& other) noexcept = delete;

	ConfigSuccess(UPoint windowSize, const std::shared_ptr<SDL_Renderer>& renderer,
				  const std::shared_ptr<TTF_Font>& font, const std::shared_ptr<SDL_Texture>& logo,
				  const std::shared_ptr<SDL_Texture>& atlas, const std::shared_ptr<SDL_Texture>& joyIcon,
				  const std::shared_ptr<SDL_Texture>& xBoxHint, const std::shared_ptr<SDL_Texture>& pS5Hint,
				  bool isVsyncOn);

	~ConfigSuccess() override = default;

	[[nodiscard]] std::unique_ptr<IGame> CreateGame() override;

	ConfigSuccess& operator=(const ConfigSuccess& other) = delete;
	ConfigSuccess& operator=(ConfigSuccess&& other) noexcept = delete;
};
