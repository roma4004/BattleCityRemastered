#pragma once

#include "geometry/Point.h"

struct ObjRectangle final
{
	double x{}, y{}, w{}, h{};

	[[nodiscard]] double Area() const;

	// Get the x-coordinate of the right side
	[[nodiscard]] double Right() const;

	// Get the y-coordinate of the bottom side
	[[nodiscard]] double Bottom() const;

	[[nodiscard]] FPoint Center() const;

	[[nodiscard]] ObjRectangle GetScaledBy(double scale) const;
};

inline double ObjRectangle::Area() const { return w * h; }

inline double ObjRectangle::Right() const { return x + w; }

inline double ObjRectangle::Bottom() const { return y + h; }

inline FPoint ObjRectangle::Center() const { return FPoint{.x = x + w / 2.0, .y = y + h / 2.0}; }

inline ObjRectangle ObjRectangle::GetScaledBy(const double scale) const
{
	ObjRectangle rectAfterScale{.x = x, .y = y, .w = w * scale, .h = h * scale};
	rectAfterScale.x -= (rectAfterScale.w - w) / 2;
	rectAfterScale.y -= (rectAfterScale.h - h) / 2;

	return rectAfterScale;
}
