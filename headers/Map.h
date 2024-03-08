#pragma once

#include "../headers/Environment.h"

class Brick;

class Map
{
public:
	Map();

	~Map();
	static void MapCreation(Environment* env, int x, int y);
};
