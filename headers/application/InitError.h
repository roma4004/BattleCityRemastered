#pragma once

#include <string>

//NOTE: both copied, not referenced - SDL_GetError() hands back a buffer the next SDL call overwrites
struct InitError
{
	std::string stage;
	std::string detail;
};
