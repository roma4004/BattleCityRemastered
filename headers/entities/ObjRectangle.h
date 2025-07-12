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

	[[nodiscard]] ObjRectangle GetCenter() const;

	[[nodiscard]] ObjRectangle GetScale(float scale) const;
};

inline float ObjRectangle::Area() const { return w * h; }

inline float ObjRectangle::Right() const { return x + w; }

inline float ObjRectangle::Bottom() const { return y + h; }

inline ObjRectangle ObjRectangle::GetCenter() const { return {x - w / 2, y - w / 2, w, h}; }

inline ObjRectangle ObjRectangle::GetScale(const float scale) const { return {x, y, w * scale, h * scale}; }

template<class Archive>
void ObjRectangle::serialize(Archive& ar, const unsigned int /*version*/)
{
	ar & x;
	ar & y;
	ar & w;
	ar & h;
}
