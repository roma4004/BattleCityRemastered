#pragma once

#include "MapData.h"
#include <expected>
#include <string>
#include <string_view>

//NOTE: parsing is split from reading so tests can feed a grid in without touching the disk
class MapLoader final
{
public:
	[[nodiscard]] static std::expected<MapData, MapError> LoadFromFile(const std::string& path);
	[[nodiscard]] static std::expected<MapData, MapError> Parse(std::string_view text, std::string path = {});
};
