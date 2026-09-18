#pragma once

#include "MapData.h"
#include <cstddef>
#include <expected>
#include <filesystem>
#include <memory>

class EventSystem;

//NOTE: the grid lives in Resources/Maps/*.map, with its legend in the file's own header comment.
class Map final
{
	std::shared_ptr<EventSystem> _events{nullptr};
	MapData _data{};

public:
	explicit Map(const std::shared_ptr<EventSystem>& events);

	[[nodiscard]] std::expected<void, MapError> LoadFromFile(const std::filesystem::path& path);

	[[nodiscard]] std::size_t GetCols() const noexcept { return _data.cols; }
	[[nodiscard]] std::size_t GetRows() const noexcept { return _data.rows; }

	void CreateObstacles(double cellSize) const;
};
