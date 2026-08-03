#include "components/Map.h"
#include "components/EventSystem.h"
#include "components/SpawnEvents.h"
#include "entities/ObjRectangle.h"
#include "enums/ObstacleType.h"

Map::Map(const std::shared_ptr<EventSystem>& events)
	: _events{events} {}

Map::~Map() = default;

void Map::ParseAndCreateObstacle(const float gridSize) const
{
	for (size_t vertical = 0ul; vertical < 52ul; ++vertical)
	{
		for (size_t horizontal = 0ul; horizontal < 50ul; ++horizontal)
		{
			const float x = static_cast<float>(vertical) * gridSize;
			const float y = static_cast<float>(horizontal) * gridSize;
			ObjRectangle rect = {.x = x, .y = y, .w = gridSize, .h = gridSize};

			switch (fieldLevelOne[horizontal][vertical])
			{
				case 0:
					break;
				case 1:
					_events->EmitEvent("SpawnObstacle", SpawnObstacleEvent{rect, ObstacleType::Brick});
					break;
				case 2:
					_events->EmitEvent("SpawnObstacle", SpawnObstacleEvent{rect, ObstacleType::Steel});
					break;
				case 3:
					rect.w += gridSize * 3;
					rect.h += gridSize * 3;
					_events->EmitEvent("SpawnObstacle", SpawnObstacleEvent{rect, ObstacleType::Eagle});
					break;
				case 4:
					_events->EmitEvent("SpawnObstacle", SpawnObstacleEvent{rect, ObstacleType::Fortress});
					break;
				case 5:
					_events->EmitEvent("SpawnObstacle", SpawnObstacleEvent{rect, ObstacleType::Water});
					break;
				case 6:
					_events->EmitEvent("SpawnObstacle", SpawnObstacleEvent{rect, ObstacleType::Bush});
					break;
				case 7:
					_events->EmitEvent("SpawnObstacle", SpawnObstacleEvent{rect, ObstacleType::Ice});
					break;
				default:
					break;
			}
		}
	}
}
