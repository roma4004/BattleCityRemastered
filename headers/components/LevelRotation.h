#pragma once

#include <string>
#include <string_view>
#include <vector>

//NOTE: letters, digits, '-' and '_' - never a path out of the maps folder, never two words on a command line
[[nodiscard]] bool IsMapName(std::string_view name);

//NOTE: the campaign is the maps folder in name order - a new level is added by dropping a file in
class LevelRotation final
{
	std::string _folder;

public:
	explicit LevelRotation(std::string folder = "Resources/Maps");

	[[nodiscard]] std::vector<std::string> Names() const;

	//NOTE: wraps from the last level to the first, so the scoreboard's next-level prompt always has a map
	[[nodiscard]] std::string PathAfter(std::string_view currentPath) const;
};
