#pragma once

#include "IGame.h"
#include <memory>

enum class GameMode : char8_t;

class IConfig
{
public:
	IConfig() = default;
	IConfig(const IConfig& other) = delete;
	IConfig(IConfig&& other) noexcept = delete;

	virtual ~IConfig() = default;

	[[nodiscard]] virtual std::unique_ptr<IGame> CreateGame(GameMode gameMode) = 0;

	IConfig& operator=(const IConfig& other) = delete;
	IConfig& operator=(IConfig&& other) noexcept = delete;
};
