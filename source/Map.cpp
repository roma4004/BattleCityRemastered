
#include "../headers/Map.h"
#include "../headers/Brick.h"


Map::Map() = default;

Map::~Map() = default;

void Map::MapCreation(Environment* env, const int x, const int y)
{
	Point brickPos = {x,y};
	env->allPawns.emplace_back(new Brick(brickPos, env));
}
