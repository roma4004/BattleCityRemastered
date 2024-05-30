#pragma once

#include "MoveLikeBulletBeh.h"
#include "Pawn.h"

struct UPoint;
class BaseObj;
class EventSystem;

class Bullet final : public Pawn
{
public:
	Bullet(const Rectangle& rect, int damage, int color, float speed, Direction direction, int health, int* windowBuffer,
		   UPoint windowSize, std::vector<std::shared_ptr<BaseObj>>* allPawns, std::shared_ptr<EventSystem> events);

	~Bullet() override;

	void Move(float deltaTime) override;
};
