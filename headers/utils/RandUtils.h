#pragma once

#include <random>
#include <chrono>

class RandUtils final
{
public:
	template<typename T>
	[[nodiscard]] static auto GetRandNumber(T distribution)
	{
		static std::random_device rd;
		static std::mt19937 gen{
				static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count() + rd())};

		return distribution(gen);
	}
};
