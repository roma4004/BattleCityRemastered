#pragma once

#include "../headers/Environment.h"

class Brick;

class Map
{
public:
	Map();

	~Map();

	static void BrickCreation(Environment* env, int x, int y);
	static void IronCreation(Environment* env, int x, int y);
};
