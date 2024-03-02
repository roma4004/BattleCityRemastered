#ifndef _BRICK_H
#define _BRICK_H

#include "BaseObj.h"
#include "IObsticle.h"

struct Point;

class Brick : public BaseObj, public IObsticle {
public:
	Brick(const Point& pos, int width, int height, int color, int speed, int health);

	~Brick() override;
};
#endif
