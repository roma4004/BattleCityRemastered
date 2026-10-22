#pragma once

struct ObjRectangle;
struct Circle;
struct UPoint;

class ColliderUtils final
{
public:
	//NOTE: an overlap below this is a touch; every gap measure must use it, or two calls disagree about one edge
	static constexpr double kTouchTolerance{0.01};

	[[nodiscard]] static bool IsCollide(const ObjRectangle& r1, const ObjRectangle& r2) noexcept;
	[[nodiscard]] static bool IsCollide(const Circle& circle, const ObjRectangle& rect) noexcept;
	[[nodiscard]] static bool IsInside(const ObjRectangle& rect, UPoint area) noexcept;
};
