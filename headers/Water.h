#pragma once

#include "BaseObj.h"

struct Point;

class Water final : public BaseObj
{
public:
	Water(const Point& pos, int width, int height, int color, float speed, int health, Environment* env);
	Water(const Point& pos, Environment* env);

	~Water() override;

	void Draw(const Environment* env) const override;
	void MarkDestroy(Environment* env) const override;
	void TickUpdate(Environment* env) override;
};