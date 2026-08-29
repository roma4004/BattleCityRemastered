#pragma once

#include <cstddef>

struct FPoint final
{
	double x{}, y{};
};

struct Point final
{
	int x{}, y{};

	//NOTE: != comes free with it since C++20
	[[nodiscard]] bool operator==(const Point& rhs) const noexcept = default;
};

struct UPoint final
{
	size_t x{}, y{};

	[[nodiscard]] bool operator==(const UPoint& rhs) const noexcept = default;
};

