#pragma once

#include <random>

class RandUtils final
{
public:
	[[nodiscard]] static int GetRandNumber(std::uniform_int_distribution<> distribution);
};
