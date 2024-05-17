#include "../headers/Map.h"
#include "../headers/Brick.h"
#include "../headers/Iron.h"
#include "../headers/Water.h"

Map::Map() = default;

Map::~Map() = default;

void Map::MapCreation(Environment* env) const
{
	for (int vertical = 0; vertical < 52; ++vertical)
	{
		for (int horizontal = 0; horizontal < 50; ++horizontal)
		{
			switch (fieldLevelOne[horizontal][vertical])
			{
				case 0:
					break;
				case 1:
					ObstacleCreation<Brick>(env, static_cast<float>(vertical) * env->gridSize,
											static_cast<float>(horizontal) * env->gridSize);
					break;
				case 2:
					ObstacleCreation<Iron>(env, static_cast<float>(vertical) * env->gridSize,
										   static_cast<float>(horizontal) * env->gridSize);
					break;
				case 3:
					ObstacleCreation<Water>(env, static_cast<float>(vertical) * env->gridSize,
											static_cast<float>(horizontal) * env->gridSize);
					break;
				default:
					break;
			}
		}
	}
}
