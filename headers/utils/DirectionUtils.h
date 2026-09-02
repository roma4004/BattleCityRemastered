#pragma once

#include "enums/Direction.h"
#include "geometry/ObjRectangle.h"
#include "geometry/Point.h"
#include <algorithm>
#include <cmath>

//NOTE: the one place that turns a direction into an axis and a sign
namespace DirectionUtils
{
[[nodiscard]] inline FPoint Unit(const Direction dir)
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

// the rectangle a step of that length sweeps through, the starting position included
[[nodiscard]] inline ObjRectangle Sweep(const ObjRectangle& rect, const double distance, const Direction dir)
{
	const auto [dx, dy] = Unit(dir);

	return ObjRectangle{.x = rect.x + std::min(dx * distance, 0.0),
						.y = rect.y + std::min(dy * distance, 0.0),
						.w = rect.w + std::abs(dx) * distance,
						.h = rect.h + std::abs(dy) * distance};
}

[[nodiscard]] inline ObjRectangle Advance(const ObjRectangle& rect, const double distance, const Direction dir)
{
	const auto [dx, dy] = Unit(dir);

	return ObjRectangle{.x = rect.x + dx * distance, .y = rect.y + dy * distance, .w = rect.w, .h = rect.h};
}

[[nodiscard]] inline FPoint Advance(const FPoint point, const double distance, const Direction dir)
{
	const auto [dx, dy] = Unit(dir);

	return FPoint{.x = point.x + dx * distance, .y = point.y + dy * distance};
}

// the side of the rectangle that lies along the movement axis
[[nodiscard]] inline double SideAlong(const ObjRectangle& rect, const Direction dir)
{
	return Unit(dir).y != 0.0 ? rect.h : rect.w;
}

// free space ahead of the leading edge - negative once the target is already behind it
[[nodiscard]] inline double GapTo(const ObjRectangle& rect, const ObjRectangle& target, const Direction dir)
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

[[nodiscard]] inline double GapToEdge(const ObjRectangle& rect, const UPoint battlefieldSize, const Direction dir)
{
	const auto [dx, dy] = Unit(dir);
	const ObjRectangle border{.x = dx > 0.0 ? static_cast<double>(battlefieldSize.x) : 0.0,
							  .y = dy > 0.0 ? static_cast<double>(battlefieldSize.y) : 0.0};

	return GapTo(rect, border, dir);
}

//NOTE: asymmetric - pixel 0 is on screen, pixel battlefieldSize is already past it
[[nodiscard]] inline bool FitsBeforeEdge(const ObjRectangle& rect, const UPoint battlefieldSize,
										 const double distance, const Direction dir)
{
	const auto [dx, dy] = Unit(dir);
	const double gap = GapToEdge(rect, battlefieldSize, dir);

	return dx + dy < 0.0 ? distance <= gap : distance < gap;
}
}// namespace DirectionUtils
