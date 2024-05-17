#pragma once

#include <SDL_rect.h>

struct Rectangle : SDL_FRect
{
public:
	[[nodiscard]] float Area() const;

	// Get the x-coordinate of the right side
	[[nodiscard]] float Right() const;

	// Get the y-coordinate of the bottom side
	[[nodiscard]] float Bottom() const;
};
