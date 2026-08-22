#include "components/Map.h"
#include "components/EventSystem.h"
#include "components/MapLoader.h"
#include "components/events/SpawnEvents.h"
#include "geometry/ObjRectangle.h"
#include "enums/ObstacleType.h"

Map::Map(const std::shared_ptr<EventSystem>& events)
	: _events{events} {}

Map::~Map() = default;

std::expected<void, MapError> Map::LoadFromFile(const std::filesystem::path& path)
{
	return MapLoader::LoadFromFile(path).transform([this](MapData data) { _data = std::move(data); });
}

void Map::CreateObstacles(const float cellSize) const
{
	for (std::size_t row = 0u; row < _data.rows; ++row)
	{
		for (std::size_t col = 0u; col < _data.cols; ++col)
		{
			//NOTE: checked here too, not only in the loader - MapData is a plain struct anyone can fill
			const ObstacleType type = _data.At(col, row);
			if (!IsSpawnableObstacle(type))
			{
				continue;
			}

			const float span = ObstacleCellSpan(type);
			const ObjRectangle rect{.x = static_cast<float>(col) * cellSize,
									.y = static_cast<float>(row) * cellSize,
									.w = cellSize * span,
									.h = cellSize * span};

			_events->EmitEvent(SpawnObstacleEvent{.rect = rect, .type = type});
		}
	}
}
