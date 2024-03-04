#include "../headers/Brick.h"

Brick::Brick(const Point& pos, const int width, const int height, const int color, const int speed, const int health)
	: BaseObj(pos, width, height, color, speed, health)
{
}

Brick::~Brick() = default;
