#pragma once

#include "BaseObj.h"

struct FPoint;

class Iron final : public BaseObj
{
public:
	Iron(const FPoint& pos, float width, float height, int color, float speed, int health, Environment* env);
	Iron(const FPoint& pos, Environment* env);

	~Iron() override;

	void Draw() const override;
	void TickUpdate() override;
};
