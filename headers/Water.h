#pragma once

#include "BaseObj.h"

struct FPoint;

class Water final : public BaseObj
{
public:
	Water(const FPoint& pos, float width, float height, int color, float speed, int health, Environment* env);
	Water(const FPoint& pos, Environment* env);

	~Water() override;

	void Draw() const override;
	void TickUpdate() override;
};
