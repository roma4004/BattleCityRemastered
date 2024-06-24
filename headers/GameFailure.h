#pragma once

#include "IGame.h"

class GameFailure final : public IGame
{
public:
	void MainLoop() override;

	[[nodiscard]] int Result() const override;
};

