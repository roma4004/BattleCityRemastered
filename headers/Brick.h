#pragma once

#include "BaseObj.h"

struct Point;

class Brick : public BaseObj
{
public:
	Brick(const Point& pos, int width, int height, int color, float speed, int health, Environment* env);

	~Brick() override;

	void Draw(const Environment* env) const override;
};