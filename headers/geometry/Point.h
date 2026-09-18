#pragma once

#include <cstddef>//NOTE: required by GCC only - std::size_t; MSVC-STL leaks it

struct FPoint final
{
	double x{}, y{};
};

struct Point final
{
	int x{}, y{};

	[[nodiscard]] bool operator==(const Point& rhs) const noexcept = default;
};

struct UPoint final
{
	size_t x{}, y{};

	[[nodiscard]] bool operator==(const UPoint& rhs) const noexcept = default;
};
