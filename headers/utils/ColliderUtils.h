#pragma once

struct ObjRectangle;
struct Circle;

class ColliderUtils final
{
public:
	[[nodiscard]] static bool IsCollide(const ObjRectangle& r1, const ObjRectangle& r2) noexcept;
	[[nodiscard]] static bool IsCollide(const Circle& circle, const ObjRectangle& rect) noexcept;
	[[nodiscard]] static bool AreEqualAbsolute(float a, float b, float epsilon = 1e-5f) noexcept;
};
