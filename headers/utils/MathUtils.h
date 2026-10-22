#pragma once

#include <concepts>

class MathUtils final
{
public:
	//NOTE: std::lround straight into the type wanted, and constexpr - MSVC's lround is not
	template<std::integral T>
	[[nodiscard]] static constexpr T RoundTo(const double value) noexcept
	{
		return static_cast<T>(value < 0.0 ? value - 0.5 : value + 0.5);
	}

	//NOTE: absolute epsilon - every value compared is a screen or world measure of the same magnitude
	[[nodiscard]] static bool AreEqualAbsolute(double a, double b, double epsilon = 1e-5) noexcept;
};
