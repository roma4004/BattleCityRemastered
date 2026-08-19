#include "utils/ColliderUtils.h"
#include "geometry/Circle.h"
#include "geometry/ObjRectangle.h"
#include <algorithm>
#include <cmath>

bool ColliderUtils::IsCollide(const ObjRectangle& r1, const ObjRectangle& r2) noexcept
{
	auto greaterThan = [](const float a, const float b)
	{
		constexpr float COLLISION_EPSILON = 0.01f;
		return a > b + COLLISION_EPSILON;
	};

	// Check if one rectangle is to the right of the other
	if (greaterThan(r1.x, r2.x + r2.w) || greaterThan(r2.x, r1.x + r1.w))
	{
		return false;
	}

	// Check if one rectangle is above the other
	if (greaterThan(r1.y, r2.y + r2.h) || greaterThan(r2.y, r1.y + r1.h))
	{
		return false;
	}

	// If neither of the above conditions is matched, the rectangles overlap
	return true;
}

bool ColliderUtils::IsCollide(const Circle& circle, const ObjRectangle& rect) noexcept
{
	const float deltaX = circle.center.x - std::max(rect.x, std::min(circle.center.x, rect.Right()));
	const float deltaY = circle.center.y - std::max(rect.y, std::min(circle.center.y, rect.Bottom()));

	return (deltaX * deltaX + deltaY * deltaY) < (circle.radius * circle.radius);
}

// Check if the absolute difference is within the allowed error margin
bool ColliderUtils::AreEqualAbsolute(const float a, const float b, const float epsilon) noexcept
{
	return std::fabs(a - b) <= epsilon;
}
