#pragma once

#include "MapData.h"
#include "enums/TankModel.h"
#include <cstddef>
#include <expected>
#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

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
	[[nodiscard]] std::optional<std::size_t> GetEnemyCount() const noexcept { return _data.enemyCount; }
	[[nodiscard]] const std::vector<TankModel>& GetEnemyLineup() const noexcept { return _data.enemyLineup; }

	//NOTE: baseless - no eagle and no wall around it
	void CreateObstacles(double cellSize, bool isBaseless) const;

	//NOTE: separate from the obstacles because a bonus is not one - it is picked up, not driven around
	void CreateBonuses(double bonusSize, double cellSize, bool isBaseless) const;
};
