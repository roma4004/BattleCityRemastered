#pragma once

#include "LaunchOptions.h"
#include <expected>
#include <string>

struct ArgError final
{
	std::string arg{};//NOTE: verbatim, so the caller echoes back what was typed
	std::string reason{};
};

//NOTE: parses only, applying is GameConfig's job
class CommandLineParser final
{
public:
	//NOTE: a malformed value stops the parse - silently ignoring "size=800x600" is
	//indistinguishable from the flag not working
	[[nodiscard]] static std::expected<LaunchOptions, ArgError> Parse(int argc, const char* const* argv);
};
