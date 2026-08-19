#pragma once

#include "MapData.h"
#include <expected>
#include <memory>

class EventSystem;

//NOTE: the grid lives in Resources/Maps/*.map, with its legend in the file's own header comment.
//The map decides its size; the window never does - see WorldGeometry for how the two are reconciled.
class Map final
{
	std::shared_ptr<EventSystem> _events{nullptr};
	MapData _data{};

public:
	explicit Map(const std::shared_ptr<EventSystem>& events);

	~Map();

	[[nodiscard]] std::expected<void, MapError> LoadFromFile(const std::filesystem::path& path);

	[[nodiscard]] std::size_t GetCols() const { return _data.cols; }
	[[nodiscard]] std::size_t GetRows() const { return _data.rows; }

	void CreateObstacles(float cellSize) const;
};
