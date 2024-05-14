#pragma once
#include "Point.h"

class Rectangle
{
public:
	FPoint pos;
	float width;
	float height;

	Rectangle(float x, float y, float w, float h);

	[[nodiscard]] float Area() const;

	// Get the x-coordinate of the right side
	[[nodiscard]] float Right() const;

	// Get the y-coordinate of the bottom side
	[[nodiscard]] float Bottom() const;
};
