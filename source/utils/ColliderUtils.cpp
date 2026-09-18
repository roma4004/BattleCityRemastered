#include "utils/ColliderUtils.h"
#include "geometry/Circle.h"
#include "geometry/ObjRectangle.h"
#include <algorithm>
#include <cmath>

bool ColliderUtils::IsCollide(const ObjRectangle& r1, const ObjRectangle& r2) noexcept
{
	auto greaterThan = [](const double a, const double b)
	{
		constexpr double COLLISION_EPSILON{0.01};
		return a > b + COLLISION_EPSILON;
	};

	if (greaterThan(r1.x, r2.x + r2.w) || greaterThan(r2.x, r1.x + r1.w))
	{
		return false;
	}

	if (greaterThan(r1.y, r2.y + r2.h) || greaterThan(r2.y, r1.y + r1.h))
	{
		return false;
	}

	return true;
}

bool ColliderUtils::IsCollide(const Circle& circle, const ObjRectangle& rect) noexcept
{
	const double deltaX{circle.center.x - std::max(rect.x, std::min(circle.center.x, rect.Right()))};
	const double deltaY{circle.center.y - std::max(rect.y, std::min(circle.center.y, rect.Bottom()))};

	return (deltaX * deltaX + deltaY * deltaY) < (circle.radius * circle.radius);
}

bool ColliderUtils::AreEqualAbsolute(const double a, const double b, const double epsilon) noexcept
{
	return std::fabs(a - b) <= epsilon;
}
