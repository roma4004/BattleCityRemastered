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

	//NOTE: absolute, not relative - everything compared here is a screen or world measure of the same
	//order, where a fixed slack reads the same at both ends of the range
	[[nodiscard]] static bool AreEqualAbsolute(double a, double b, double epsilon = 1e-5) noexcept;
};
