#pragma once

#include "../Point.h"

#include <memory>

struct Window final
{
	UPoint size{.x = 800, .y = 600};
	std::shared_ptr<int[]> buffer{nullptr};

	explicit Window(UPoint windowSize);
	Window(UPoint windowSize, std::shared_ptr<int[]> buffer);
};
