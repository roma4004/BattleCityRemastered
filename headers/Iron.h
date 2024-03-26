#pragma once

#include "BaseObj.h"

struct Point;

class Iron final : public BaseObj
{
public:
	Iron(const Point& pos, int width, int height, int color, float speed, int health, Environment* env);
	Iron(const Point& pos, Environment* env);

	~Iron() override;

	void Draw() const override;
	void TickUpdate() override;
};
