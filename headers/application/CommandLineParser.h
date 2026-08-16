#pragma once

#include "LaunchOptions.h"

//NOTE: parses only, applying is GameConfig's job
class CommandLineParser final
{
public:
	[[nodiscard]] static LaunchOptions Parse(int argc, const char* const* argv);
};
