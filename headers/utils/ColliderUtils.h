#pragma once

struct ObjRectangle;
struct Circle;

class ColliderUtils final
{
public:
	[[nodiscard]] static bool IsCollide(const ObjRectangle& r1, const ObjRectangle& r2);
	[[nodiscard]] static bool IsCollide(const Circle& circle, const ObjRectangle& rect);
};
