#pragma once

#include "enums/Direction.h"
#include "geometry/ObjRectangle.h"
#include "geometry/Point.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace DirectionUtils
{
[[nodiscard]] constexpr FPoint Unit(const Direction dir) noexcept
{
	switch (dir)
	{
		case Direction::UP:
			return FPoint{.x = 0.0, .y = -1.0};
		case Direction::LEFT:
			return FPoint{.x = -1.0, .y = 0.0};
		case Direction::DOWN:
			return FPoint{.x = 0.0, .y = 1.0};
		case Direction::RIGHT:
			return FPoint{.x = 1.0, .y = 0.0};
	}

	return FPoint{};
}

[[nodiscard]] constexpr Direction Opposite(const Direction dir) noexcept
{
	switch (dir)
	{
		case Direction::UP:
			return Direction::DOWN;
		case Direction::LEFT:
			return Direction::RIGHT;
		case Direction::DOWN:
			return Direction::UP;
		case Direction::RIGHT:
			return Direction::LEFT;
	}

	return dir;
}

//NOTE: the two ways across the way it is going - where a tank can be nudged without changing its heading
[[nodiscard]] constexpr std::array<Direction, 2> Laterals(const Direction dir) noexcept
{
	if (dir == Direction::UP || dir == Direction::DOWN)
	{
		return {Direction::LEFT, Direction::RIGHT};
	}

	return {Direction::UP, Direction::DOWN};
}

// the rectangle a step of that length sweeps through, the starting position included
[[nodiscard]] inline ObjRectangle Swept(const ObjRectangle& rect, const double distance, const Direction dir) noexcept
{
	const auto [dx, dy] = Unit(dir);

	return ObjRectangle{.x = rect.x + std::min(dx * distance, 0.0),
						.y = rect.y + std::min(dy * distance, 0.0),
						.w = rect.w + std::abs(dx) * distance,
						.h = rect.h + std::abs(dy) * distance};
}

[[nodiscard]] constexpr ObjRectangle Moved(const ObjRectangle& rect, const double distance,
										  const Direction dir) noexcept
{
	const auto [dx, dy] = Unit(dir);

	return ObjRectangle{.x = rect.x + dx * distance, .y = rect.y + dy * distance, .w = rect.w, .h = rect.h};
}

[[nodiscard]] constexpr FPoint Moved(const FPoint point, const double distance, const Direction dir) noexcept
{
	const auto [dx, dy] = Unit(dir);

	return FPoint{.x = point.x + dx * distance, .y = point.y + dy * distance};
}

[[nodiscard]] constexpr double SizeAlong(const ObjRectangle& rect, const Direction dir) noexcept
{
	return dir == Direction::UP || dir == Direction::DOWN ? rect.h : rect.w;
}

// free space ahead of the leading edge - negative once the target is already behind it
[[nodiscard]] constexpr double GapTo(const ObjRectangle& rect, const ObjRectangle& target, const Direction dir) noexcept
{
	switch (dir)
	{
		case Direction::UP:
			return rect.y - target.Bottom();
		case Direction::LEFT:
			return rect.x - target.Right();
		case Direction::DOWN:
			return target.y - rect.Bottom();
		case Direction::RIGHT:
			return target.x - rect.Right();
	}

	return 0.0;
}

[[nodiscard]] constexpr double GapToEdge(const ObjRectangle& rect, const UPoint battlefieldSize,
										 const Direction dir) noexcept
{
	const auto [dx, dy] = Unit(dir);
	const ObjRectangle border{.x = dx > 0.0 ? static_cast<double>(battlefieldSize.x) : 0.0,
							  .y = dy > 0.0 ? static_cast<double>(battlefieldSize.y) : 0.0};

	return GapTo(rect, border, dir);
}

//NOTE: asymmetric - pixel 0 is on screen, pixel battlefieldSize is already past it
[[nodiscard]] constexpr bool FitsBeforeEdge(const ObjRectangle& rect, const UPoint battlefieldSize,
											const double distance, const Direction dir) noexcept
{
	const auto [dx, dy] = Unit(dir);
	const double gap{GapToEdge(rect, battlefieldSize, dir)};

	return dx + dy < 0.0 ? distance <= gap : distance < gap;
}
}// namespace DirectionUtils
