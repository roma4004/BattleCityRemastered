#pragma once

#include "BaseObj.h"

struct Point;

class Brick final : public BaseObj
{
public:
	Brick(const Point& pos, int width, int height, int color, float speed, int health, Environment* env);
	Brick(const Point& pos, Environment* env);

	~Brick() override;

	void Draw() const override;
	void TickUpdate() override;
};
