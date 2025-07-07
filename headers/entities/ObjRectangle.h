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

// Include the template implementation
#include "ObjRectangle.tpp"
