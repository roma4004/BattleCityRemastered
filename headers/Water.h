#pragma once

#include "BaseObj.h"

struct Point;

class Water final : public BaseObj
{
public:
	Water(const Point& pos, int width, int height, int color, float speed, int health, Environment* env);
	Water(const Point& pos, Environment* env);

	~Water() override;

	void Draw() const override;
	void TickUpdate() override;
};
