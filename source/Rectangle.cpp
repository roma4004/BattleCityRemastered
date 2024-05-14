#include "Rectangle.h"

Rectangle::Rectangle(const float x, const float y, const float w, const float h)
	: pos{x, y}, width(w), height(h)
{

}

float Rectangle::Area() const
{
	return width * height;
}

float Rectangle::Right() const
{
	return pos.x + width;
}

float Rectangle::Bottom() const
{
	return pos.y + height;
}