#include "../../headers/utils/ColliderUtils.h"
#include "../../headers/Circle.h"
#include "../../headers/ObjRectangle.h"

#include <algorithm>

bool ColliderUtils::IsCollide(const ObjRectangle& r1, const ObjRectangle& r2)
{
	// Check if one rectangle is to the right of the other
	if (r1.x > r2.x + r2.w || r2.x > r1.x + r1.w)
	{
		return false;
	}

	// Check if one rectangle is above the other
	if (r1.y > r2.y + r2.h || r2.y > r1.y + r1.h)
	{
		return false;
	}

	// If neither of the above conditions are met, the rectangles overlap
	return true;
}

bool ColliderUtils::IsCollide(const Circle& circle, const ObjRectangle& rect)
{
	const double deltaX = circle.center.x - std::max(rect.x, std::min(circle.center.x, rect.Right()));
	const double deltaY = circle.center.y - std::max(rect.y, std::min(circle.center.y, rect.Bottom()));

	return (deltaX * deltaX + deltaY * deltaY) < (circle.radius * circle.radius);
}
