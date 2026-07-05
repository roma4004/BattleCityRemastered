#pragma once

#include "GameConfig.h"
#include "Point.h"
#include "interfaces/IConfig.h"
#include <SDL.h>
#include <SDL_ttf.h>

enum class GameMode : char8_t;

class ConfigSuccess final : public IConfig
{
	UPoint _windowSize{};
	boost::property_tree::ptree& _pTreeIni;
	std::vector<std::shared_ptr<SDL_Texture>>& buttonTexturesPS5;
	std::vector<std::shared_ptr<SDL_Texture>>& buttonTexturesXBox;
	std::shared_ptr<SDL_Renderer> _renderer{nullptr};
	std::shared_ptr<TTF_Font> _fontSmall{nullptr};
	std::shared_ptr<TTF_Font> _fontMedium{nullptr};
	std::shared_ptr<SDL_Texture> _logo{nullptr};
	std::shared_ptr<SDL_Texture> _atlas{nullptr};
	std::shared_ptr<SDL_Texture> _selectorIcon{nullptr};

public:
	ConfigSuccess() = delete;
	ConfigSuccess(const ConfigSuccess& other) = delete;
	ConfigSuccess(ConfigSuccess&& other) noexcept = delete;

	ConfigSuccess(UPoint windowSize, boost::property_tree::ptree& pTreeIni,
				  std::vector<std::shared_ptr<SDL_Texture>>& buttonTexturesPS5,
				  std::vector<std::shared_ptr<SDL_Texture>>& buttonTexturesXBox,
				  const std::shared_ptr<SDL_Renderer>& renderer,
				  const std::shared_ptr<TTF_Font>& fontSmall, const std::shared_ptr<TTF_Font>& fontMedium,
				  const std::shared_ptr<SDL_Texture>& logo, const std::shared_ptr<SDL_Texture>& atlas,
				  const std::shared_ptr<SDL_Texture>& selectorIcon);

	~ConfigSuccess() override = default;

	[[nodiscard]] std::unique_ptr<IGame> CreateGame(GameMode gameMode) override;

	ConfigSuccess& operator=(const ConfigSuccess& other) = delete;
	ConfigSuccess& operator=(ConfigSuccess&& other) noexcept = delete;
};
