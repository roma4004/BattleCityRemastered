#pragma once

struct ObjRectangle final
{
	float x{}, y{}, w{}, h{};

	template<class Archive>
	void serialize(Archive& ar, unsigned int version);

	[[nodiscard]] float Area() const;

	// Get the x-coordinate of the right side
	[[nodiscard]] float Right() const;

	// Get the y-coordinate of the bottom side
	[[nodiscard]] float Bottom() const;

	[[nodiscard]] ObjRectangle GetScaledBy(float scale) const;
};

inline float ObjRectangle::Area() const { return w * h; }

inline float ObjRectangle::Right() const { return x + w; }

inline float ObjRectangle::Bottom() const { return y + h; }

inline ObjRectangle ObjRectangle::GetScaledBy(const float scale) const
{
	ObjRectangle rectAfterScale = {.x = x, .y = y, .w = w * scale, .h = h * scale};
	rectAfterScale.x -= (rectAfterScale.w - w) / 2;
	rectAfterScale.y -= (rectAfterScale.h - h) / 2;

	return rectAfterScale;
}

template<class Archive>
void ObjRectangle::serialize(Archive& ar, const unsigned int /*version*/)
{
	ar & x;
	ar & y;
	ar & w;
	ar & h;
}
