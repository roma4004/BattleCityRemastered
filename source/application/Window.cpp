#include "../../headers/application/Window.h"

Window::Window(const UPoint windowSize)
	: size{windowSize},
	  buffer{new int[windowSize.x * windowSize.y],
			 std::default_delete<int[]>()} {}

Window::Window(UPoint windowSize, std::shared_ptr<int[]> buffer)
	: size{std::move(windowSize)},
	  buffer{std::move(buffer)} {}
