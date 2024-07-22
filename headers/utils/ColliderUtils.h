#pragma once

#include "../Circle.h"
#include "../ObjRectangle.h"

class ColliderUtils
{
public:
	[[nodiscard]] static bool IsCollide(const ObjRectangle& r1, const ObjRectangle& r2);
	[[nodiscard]] static bool IsCollide(const Circle& circle, const ObjRectangle& rect);
};
