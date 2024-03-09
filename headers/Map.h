#pragma once

#include "../headers/Environment.h"

class Map
{
public:
	Map();

	~Map();

	template <typename T>
	static void BrickCreation(Environment* env, const int x, const int y);
};

template<typename T>
void Map::BrickCreation(Environment* env, const int x, const int y) {
	for (int vertical = 0; vertical < 2; ++vertical) {
		for (int horizontal = 0; horizontal < 2; ++horizontal) {
			Point brickPos = {x + (horizontal * 10),y + (vertical * 10)};

			env->allPawns.emplace_back(new T(brickPos, env));
		}
	}
}

