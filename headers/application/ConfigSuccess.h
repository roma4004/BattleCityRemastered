#pragma once

#include "interfaces/IConfig.h"

enum class GameMode : char8_t;
struct SDL_Config;
class GameConfig;

class ConfigSuccess final : public IConfig
{
	GameConfig& _gameConfig;

public:
	ConfigSuccess() = delete;
	ConfigSuccess(const ConfigSuccess& other) = delete;
	ConfigSuccess(ConfigSuccess&& other) noexcept = delete;

	explicit ConfigSuccess(GameConfig& gameConfig);

	~ConfigSuccess() override = default;

	[[nodiscard]] std::unique_ptr<IGame> CreateGame(GameMode gameMod, SDL_Config& sdlConfig) override;

	ConfigSuccess& operator=(const ConfigSuccess& other) = delete;
	ConfigSuccess& operator=(ConfigSuccess&& other) noexcept = delete;
};
