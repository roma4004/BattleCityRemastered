#include "../headers/Map.h"
#include "../headers/components/ObstacleSpawner.h"
#include "../headers/enums/ObstacleType.h"

Map::Map(std::shared_ptr<ObstacleSpawner> obstacleSpawner) : _obstacleSpawner{std::move(obstacleSpawner)} {}

Map::~Map() {}

void Map::MapCreation(const float gridSize) const
{
	for (int vertical = 0; vertical < 52; ++vertical)
	{
		for (int horizontal = 0; horizontal < 50; ++horizontal)
		{
			const float x = static_cast<float>(vertical) * gridSize;
			const float y = static_cast<float>(horizontal) * gridSize;
			ObjRectangle rect = {.x = x, .y = y, .w = gridSize, .h = gridSize};

			switch (fieldLevelOne[horizontal][vertical])
			{
				case 0:
					break;
				case 1:
					_obstacleSpawner->SpawnObstacle(rect, Brick);
					break;
				case 2:
					_obstacleSpawner->SpawnObstacle(rect, Steel);
					break;
				case 3:
					rect.w += gridSize * 3;
					rect.h += gridSize * 3;
					_obstacleSpawner->SpawnObstacle(rect, Eagle);
					break;
				case 4:
					_obstacleSpawner->SpawnObstacle(rect, Fortress);
					break;
				case 5:
					_obstacleSpawner->SpawnObstacle(rect, Water);
					break;
				case 6:
					_obstacleSpawner->SpawnObstacle(rect, Grass);
					break;
				case 7:
					_obstacleSpawner->SpawnObstacle(rect, Ice);
					break;
				default:
					break;
			}
		}
	}
}
