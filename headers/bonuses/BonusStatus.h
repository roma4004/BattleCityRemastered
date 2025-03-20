#pragma once

#include <chrono>

struct BonusStatus
{
	bool isActive{false};
	std::chrono::milliseconds cooldown{0};
	std::chrono::system_clock::time_point activateTime;

	BonusStatus();

	BonusStatus(bool isActive, std::chrono::milliseconds cooldown, std::chrono::system_clock::time_point activateTime);

	~BonusStatus();
};
