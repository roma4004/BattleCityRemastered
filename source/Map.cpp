#include "../headers/Map.h"
#include "../headers/Brick.h"
#include "../headers/Iron.h"
#include "../headers/Water.h"

Map::Map() = default;

Map::~Map() = default;

void Map::MapCreation(std::vector<std::shared_ptr<BaseObj>>* allPawns, float gridSize, int* windowBuffer,
					  size_t windowWidth, size_t windowHeight, const std::shared_ptr<EventSystem>& events) const
{
	for (int vertical = 0; vertical < 52; ++vertical)
	{
		for (int horizontal = 0; horizontal < 50; ++horizontal)
		{
			const float x = static_cast<float>(vertical) * gridSize;
			const float y = static_cast<float>(horizontal) * gridSize;
			switch (fieldLevelOne[horizontal][vertical])
			{
				case 0:
					break;
				case 1:
					ObstacleCreation<Brick>(allPawns, x, y, gridSize, windowBuffer, windowWidth, windowHeight, events);
					break;
				case 2:
					ObstacleCreation<Iron>(allPawns, x, y, gridSize, windowBuffer, windowWidth, windowHeight, events);
					break;
				case 3:
					ObstacleCreation<Water>(allPawns, x, y, gridSize, windowBuffer, windowWidth, windowHeight, events);
					break;
				default:
					break;
			}
		}
	}
}
