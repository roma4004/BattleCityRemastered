#include "entities/ObjRectangle.h"

float ObjRectangle::Area() const { return w * h; }

float ObjRectangle::Right() const { return x + w; }

float ObjRectangle::Bottom() const { return y + h; }

ObjRectangle ObjRectangle::GetCenter() const
{
	return {x - w / 2, y - w / 2, w, h};
}

ObjRectangle ObjRectangle::GetScale(const float scale) const
{
	return {x, y, w * scale, h * scale};
}
