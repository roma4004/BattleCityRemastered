#include "../headers/Map.h"
#include "../headers/components/ObstacleSpawner.h"
#include "../headers/enums/ObstacleType.h"

Map::Map(ObstacleSpawner* obstacleSpawner) : _obstacleSpawner{obstacleSpawner} {}

Map::~Map() {}

void Map::MapCreation(const float gridSize) const
{
	for (int vertical = 0; vertical < 52; ++vertical)
	{
		for (int horizontal = 0; horizontal < 50; ++horizontal)
		{
			const float x = static_cast<float>(vertical) * gridSize;
			const float y = static_cast<float>(horizontal) * gridSize;
			ObjRectangle rect = {.x = x, .y = y, .w = gridSize - 1, .h = gridSize - 1};
			switch (fieldLevelOne[horizontal][vertical])
			{
				case 0:
					break;
				case 1:
					_obstacleSpawner->SpawnObstacle(std::move(rect), Brick);
					break;
				case 2:
					_obstacleSpawner->SpawnObstacle(std::move(rect), Steel);
					break;
				case 3:
					rect.w += 1;
					rect.h += 1;
					_obstacleSpawner->SpawnObstacle(std::move(rect), Water);
					break;
				case 4:
					_obstacleSpawner->SpawnObstacle(std::move(rect), Fortress);
					break;
				default:
					break;
			}
		}
	}
}
