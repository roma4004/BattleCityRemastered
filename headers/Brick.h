#pragma once

#include "BaseObj.h"

struct FPoint;

class Brick final : public BaseObj
{
public:
	Brick(const FPoint& pos, float width, float height, int color, float speed, int health, Environment* env);
	Brick(const FPoint& pos, Environment* env);

	~Brick() override;

	void Draw() const override;
	void TickUpdate() override;
};
