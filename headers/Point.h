#pragma once

#include <cmath>

struct FPoint
{
	float x{}, y{};

	bool operator==(const FPoint& rhs) const
	{
		static constexpr float epsilon = 1e-4f;
		return std::abs(x - rhs.x) < epsilon && std::abs(y - rhs.y) < epsilon;
	}

	bool operator!=(const FPoint& rhs) const { return !(*this == rhs); }

	bool operator<(const FPoint& rhs) const
	{
		static constexpr float epsilon = 1e-4f;
		return x < rhs.x && std::abs(x - rhs.x) > epsilon ||
			   (std::abs(x - rhs.x) < epsilon && y < rhs.y && std::abs(y - rhs.y) > epsilon);
	}

	bool operator>(const FPoint& rhs) const { return rhs < *this; }

	bool operator<=(const FPoint& rhs) const { return *this == rhs || *this < rhs; }

	bool operator>=(const FPoint& rhs) const { return *this == rhs || *this > rhs; }
};

struct Point
{
	int x{}, y{};
};

struct UPoint
{
	size_t x{}, y{};
};
