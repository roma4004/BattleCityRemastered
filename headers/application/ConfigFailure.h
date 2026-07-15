#pragma once

#include "interfaces/IConfig.h"
#include <string>

enum class GameMode : char8_t;
struct SDL_Config;

class ConfigFailure final : public IConfig
{
	std::string _error{};
	const char* _description{};

public:
	ConfigFailure() = delete;
	ConfigFailure(const ConfigFailure& other) = delete;
	ConfigFailure(ConfigFailure&& other) noexcept = delete;

	ConfigFailure(std::string errorType, const char* errorDescription);

	~ConfigFailure() override = default;

	[[nodiscard]] std::unique_ptr<IGame> CreateGame(GameMode gameMode, SDL_Config& sdlConfig) override;

	ConfigFailure& operator=(const ConfigFailure& other) = delete;
	ConfigFailure& operator=(ConfigFailure&& other) noexcept = delete;
};
