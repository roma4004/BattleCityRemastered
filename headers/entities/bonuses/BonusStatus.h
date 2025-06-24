#pragma once

#include <chrono>

struct BonusStatus
{
	using milliseconds = std::chrono::milliseconds;

	milliseconds cooldown{0};
	std::chrono::system_clock::time_point activateTime{};
	bool isActive{false};

	BonusStatus();

	BonusStatus(bool isActive, milliseconds cooldown, std::chrono::system_clock::time_point activateTime);

	~BonusStatus();
};
