#pragma once

#include "geometry/Point.h"

struct ObjRectangle final
{
	float x{}, y{}, w{}, h{};

	[[nodiscard]] float Area() const;

	// Get the x-coordinate of the right side
	[[nodiscard]] float Right() const;

	// Get the y-coordinate of the bottom side
	[[nodiscard]] float Bottom() const;

	[[nodiscard]] FPoint Center() const;

	[[nodiscard]] ObjRectangle GetScaledBy(float scale) const;
};

inline float ObjRectangle::Area() const { return w * h; }

inline float ObjRectangle::Right() const { return x + w; }

inline float ObjRectangle::Bottom() const { return y + h; }

inline FPoint ObjRectangle::Center() const { return FPoint{.x = x + w / 2.f, .y = y + h / 2.f}; }

inline ObjRectangle ObjRectangle::GetScaledBy(const float scale) const
{
	ObjRectangle rectAfterScale = {.x = x, .y = y, .w = w * scale, .h = h * scale};
	rectAfterScale.x -= (rectAfterScale.w - w) / 2;
	rectAfterScale.y -= (rectAfterScale.h - h) / 2;

	return rectAfterScale;
}
