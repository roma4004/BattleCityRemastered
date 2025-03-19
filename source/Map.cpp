#include "../headers/Map.h"
#include "../headers/obstacles/ObstacleSpawner.h"
#include "../headers/obstacles/ObstacleTypeId.h"

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
			const ObjRectangle rect = {.x = x, .y = y, .w = gridSize, .h = gridSize};
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
					_obstacleSpawner->SpawnObstacle(rect, Water);
					break;
				case 4:
					_obstacleSpawner->SpawnObstacle(rect, Fortress);
					break;
				default:
					break;
			}
		}
	}
}
