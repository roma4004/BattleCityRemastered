#pragma once

struct ObjRectangle final
{
	float x{}, y{}, w{}, h{};

	[[nodiscard]] float Area() const;

	// Get the x-coordinate of the right side
	[[nodiscard]] float Right() const;

	// Get the y-coordinate of the bottom side
	[[nodiscard]] float Bottom() const;
};
