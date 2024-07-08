#pragma once

#include "../Circle.h"
#include "../Rectangle.h"

class ColliderCheck
{
public:
	[[nodiscard]] static bool IsCollide(const Rectangle& r1, const Rectangle& r2);
	[[nodiscard]] static bool IsCollide(const Circle& circle, const Rectangle& rect);
};
