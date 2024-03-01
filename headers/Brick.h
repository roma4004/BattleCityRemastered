#pragma once

#include "BaseObj.h"
#include "IObsticle.h"

class Brick : public BaseObj, public IObsticle {
public:
	Brick(const Point &pos, int width, int height, int color, int speed, int health);

	~Brick() override;
};