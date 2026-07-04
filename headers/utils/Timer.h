#pragma once

#include <chrono>

struct Timer
{
	using milliseconds = std::chrono::milliseconds;

	milliseconds cooldown{0};
	std::chrono::system_clock::time_point activateTime{};
	bool isActive{false};

	Timer();

	Timer(milliseconds cooldown, std::chrono::system_clock::time_point activateTime);

	~Timer();

	[[nodiscard]] bool IsCooldownFinish() const;

	void Reset();
	void Reset(milliseconds newCooldown);
};
