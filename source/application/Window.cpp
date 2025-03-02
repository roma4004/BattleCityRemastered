#include "../../headers/application/Window.h"

Window::Window(UPoint windowSize, std::shared_ptr<int[]> buffer)
	: size{std::move(windowSize)},
	  buffer{std::move(buffer)} {}
