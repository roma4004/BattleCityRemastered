#pragma once

#include <chrono>

struct BonusStatus
{
	using milliseconds = std::chrono::milliseconds;

	bool isActive{false};
	milliseconds cooldown{0};
	std::chrono::system_clock::time_point activateTime;

	BonusStatus();

	BonusStatus(bool isActive, milliseconds cooldown, std::chrono::system_clock::time_point activateTime);

	~BonusStatus();
};
