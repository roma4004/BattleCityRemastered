#pragma once

#include "geometry/Point.h"

struct ObjRectangle final
{
	double x{}, y{}, w{}, h{};

	[[nodiscard]] constexpr double Area() const noexcept;

	[[nodiscard]] constexpr double Right() const noexcept;

	[[nodiscard]] constexpr double Bottom() const noexcept;

	[[nodiscard]] constexpr FPoint Center() const noexcept;

	[[nodiscard]] constexpr ObjRectangle GetScaledBy(double scale) const noexcept;
};

constexpr double ObjRectangle::Area() const noexcept { return w * h; }

constexpr double ObjRectangle::Right() const noexcept { return x + w; }

constexpr double ObjRectangle::Bottom() const noexcept { return y + h; }

constexpr FPoint ObjRectangle::Center() const noexcept { return FPoint{.x = x + w / 2.0, .y = y + h / 2.0}; }

constexpr ObjRectangle ObjRectangle::GetScaledBy(const double scale) const noexcept
{
	ObjRectangle rectAfterScale{.x = x, .y = y, .w = w * scale, .h = h * scale};
	rectAfterScale.x -= (rectAfterScale.w - w) / 2;
	rectAfterScale.y -= (rectAfterScale.h - h) / 2;

	return rectAfterScale;
}
