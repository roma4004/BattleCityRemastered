#include "../headers/Map.h"
#include "../headers/obstacles/Brick.h"
#include "../headers/obstacles/ObstacleAroundFortress.h"
#include "../headers/obstacles/Iron.h"
#include "../headers/obstacles/Water.h"

Map::Map() = default;

Map::~Map() = default;

void Map::MapCreation(std::vector<std::shared_ptr<BaseObj>>* allObjects, const float gridSize, int* windowBuffer,
                      const UPoint windowSize, const std::shared_ptr<EventSystem>& events) const
{
	for (int vertical = 0; vertical < 52; ++vertical)
	{
		for (int horizontal = 0; horizontal < 50; ++horizontal)
		{
			const float x = static_cast<float>(vertical) * gridSize;
			const float y = static_cast<float>(horizontal) * gridSize;
			Rectangle rect = {x, y, gridSize, gridSize};
			switch (fieldLevelOne[horizontal][vertical])
			{
				case 0:
					break;
				case 1:
					ObstacleCreation<Brick>(allObjects, rect, windowBuffer, windowSize, events);
					break;
				case 2:
					ObstacleCreation<Iron>(allObjects, rect, windowBuffer, windowSize, events);
					break;
				case 3:
					ObstacleCreation<Water>(allObjects, rect, windowBuffer, windowSize, events);
					break;
				case 4:
					ObstacleCreation<ObstacleAroundFortress>(allObjects, rect, windowBuffer, windowSize, events);
					break;
				default:
					break;
			}
		}
	}
}
