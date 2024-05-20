#pragma once

#include "Game.h"

#include <memory>

class Config
{
public:
	Config() = default;
	Config(const Config& other) = delete;
	Config(Config&& other) noexcept = delete;

	virtual ~Config() = default;

	[[nodiscard]] virtual std::unique_ptr<Game> CreateGame() = 0;

	Config& operator=(const Config& other) = delete;
	Config& operator=(Config&& other) noexcept = delete;
};
