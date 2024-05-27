#pragma once

#include "Config.h"

#include <string>

class ConfigFailure final : public Config
{
	std::string _error;
	const char* _description;

public:
	ConfigFailure() = delete;
	ConfigFailure(const ConfigFailure& other) = delete;
	ConfigFailure(ConfigFailure&& other) noexcept = delete;

	ConfigFailure(std::string errorType, const char* errorDescription);

	~ConfigFailure() override = default;

	[[nodiscard]] std::unique_ptr<Game> CreateGame() override;

	ConfigFailure& operator=(const ConfigFailure& other) = delete;
	ConfigFailure& operator=(ConfigFailure&& other) noexcept = delete;
};
