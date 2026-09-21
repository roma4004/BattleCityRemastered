#pragma once

#include "MapData.h"
#include <expected>
#include <filesystem>
#include <string>
#include <string_view>

//NOTE: parsing is split from reading so tests can feed a grid in without touching the disk
class MapLoader final
{
public:
	[[nodiscard]] static std::expected<MapData, MapError> LoadFromFile(const std::filesystem::path& path);
	[[nodiscard]] static std::expected<MapData, MapError> Parse(std::string_view text,
																std::filesystem::path path = {});

	//NOTE: the rules of the game, not of the grid - Parse reads the picture, this asks whether a match can
	//be played on it. Separate so a test can feed either half its own grid
	[[nodiscard]] static std::expected<void, MapError> Validate(const MapData& map,
															   std::filesystem::path path = {});
};
