#include "../headers/Map.h"
#include "../headers/obstacles/Brick.h"
#include "../headers/obstacles/FortressObstacle.h"
#include "../headers/obstacles/Iron.h"
#include "../headers/obstacles/Water.h"

Map::Map() = default;

Map::~Map() = default;

void Map::MapCreation(std::vector<std::shared_ptr<BaseObj>>* allObjects, const float gridSize, int* windowBuffer,
                      const UPoint windowSize, const std::shared_ptr<EventSystem>& events) const
{
	int obstacleId = 0;
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
					ObstacleCreation<Brick>(allObjects, rect, windowBuffer, windowSize, events, obstacleId++);
					break;
				case 2:
					ObstacleCreation<Iron>(allObjects, rect, windowBuffer, windowSize, events, obstacleId++);
					break;
				case 3:
					ObstacleCreation<Water>(allObjects, rect, windowBuffer, windowSize, events, obstacleId++);
					break;
				case 4:
					ObstacleCreation<
						FortressObstacle>(allObjects, rect, windowBuffer, windowSize, events, obstacleId++);
					break;
				default:
					break;
			}
		}
	}
}
