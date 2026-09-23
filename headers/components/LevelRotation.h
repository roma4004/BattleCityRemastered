#pragma once

#include <string>
#include <string_view>
#include <vector>

//NOTE: the campaign is the maps folder itself, read in name order - a new level joins by being dropped in,
//and nothing in the code lists the levels by hand
class LevelRotation final
{
	std::string _folder;

public:
	explicit LevelRotation(std::string folder = "Resources/Maps");

	[[nodiscard]] std::vector<std::string> Names() const;

	//NOTE: the last level is followed by the first one again - a campaign that ran out would leave the
	//prompt on the scoreboard pointing at nothing
	[[nodiscard]] std::string PathAfter(std::string_view currentPath) const;
};
