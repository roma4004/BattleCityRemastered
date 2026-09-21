#pragma once

struct ObjRectangle;
struct Circle;

class ColliderUtils final
{
public:
	//NOTE: shallower than this and the two are touching, not overlapping - whoever measures a gap has to
	//read it the same way, or an edge one call calls clear the other calls behind it
	static constexpr double kTouchTolerance{0.01};

	[[nodiscard]] static bool IsCollide(const ObjRectangle& r1, const ObjRectangle& r2) noexcept;
	[[nodiscard]] static bool IsCollide(const Circle& circle, const ObjRectangle& rect) noexcept;
	[[nodiscard]] static bool AreEqualAbsolute(double a, double b, double epsilon = 1e-5) noexcept;
};
