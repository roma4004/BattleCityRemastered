
#include "../headers/Map.h"
#include "../headers/Brick.h"


Map::Map() = default;

Map::~Map() = default;


void Map::BrickCreation(Environment* env, const int x, const int y)
{
	for (int vertical = 0; vertical < 2; ++vertical) {
		for (int horizontal = 0; horizontal < 2; ++horizontal) {
			Point brickPos = {x + (horizontal * 10),y + (vertical * 10)};
			env->allPawns.emplace_back(new Brick(brickPos, env));
		}
	}
}

void Map::IronCreation(Environment* env, const int x, const int y)
{
	for (int vertical = 0; vertical < 2; ++vertical) {
		for (int horizontal = 0; horizontal < 2; ++horizontal) {
			Point brickPos = {x + (horizontal * 10),y + (vertical * 10)};
			env->allPawns.emplace_back(new Iron(brickPos, env));
		}
	}
}

