#pragma once

#include "LaunchOptions.h"
#include <expected>
#include <string>

struct ArgError final
{
	std::string arg{};//NOTE: verbatim, so the caller echoes back what was typed
	std::string reason{};
};

class CommandLineParser final
{
public:
	//NOTE: a malformed value such as "size=800x600" fails the whole parse, so a typo never passes for a dead flag
	[[nodiscard]] static std::expected<LaunchOptions, ArgError> Parse(int argc, const char* const* argv);
};
