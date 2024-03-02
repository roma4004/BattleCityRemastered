#include "Brick.h"

Brick::Brick(const Point& pos, int width, int height, int color, int speed, int health)
	: BaseObj(pos, width, height, color, speed, health) {}

Brick::~Brick() = default;
