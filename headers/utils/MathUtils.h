#pragma once

class MathUtils final
{
public:
	//NOTE: absolute, not relative - everything compared here is a screen or world measure of the same
	//order, where a fixed slack reads the same at both ends of the range
	[[nodiscard]] static bool AreEqualAbsolute(double a, double b, double epsilon = 1e-5) noexcept;
};
