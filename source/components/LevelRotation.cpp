#include "components/LevelRotation.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string_view>
#include <system_error>
#include <utility>

namespace
{
constexpr std::string_view kMapExtension{".map"};
}//namespace

bool IsMapName(const std::string_view name)
{
	const auto isAllowed = [](const char symbol)
	{
		return std::isalnum(static_cast<unsigned char>(symbol)) != 0 || symbol == '-' || symbol == '_';
	};

	return !name.empty() && std::ranges::all_of(name, isAllowed);
}

LevelRotation::LevelRotation(std::string folder)
	: _folder{std::move(folder)} {}

std::vector<std::string> LevelRotation::Names() const
{
	std::vector<std::string> names{};

	//NOTE: the error code is the guard, not the loop - a missing folder answers with an empty listing
	//instead of throwing at whoever asked which levels there are
	std::error_code ec;
	for (const auto& entry: std::filesystem::directory_iterator{_folder, ec})
	{
		if (entry.path().extension() == kMapExtension)
		{
			names.push_back(entry.path().stem().string());
		}
	}

	std::ranges::sort(names);

	return names;
}

std::string LevelRotation::PathAfter(const std::string_view currentPath) const
{
	const std::vector<std::string> names{Names()};
	if (names.empty())
	{
		return std::string{currentPath};
	}

	const std::string current{std::filesystem::path{currentPath}.stem().string()};
	const auto found{std::ranges::find(names, current)};
	const auto next{found == names.end() || std::next(found) == names.end() ? names.begin() : std::next(found)};

	return (std::filesystem::path{_folder} / (*next + std::string{kMapExtension})).generic_string();
}
