#pragma once

#include "MoveLikeBulletBeh.h"
#include "Pawn.h"

#include <string>

struct UPoint;
class BaseObj;
class EventSystem;

class Bullet : public Pawn
{
	std::string _name;

public:
	Bullet(const Rectangle& rect, int damage, double aoeRadius, int color, float speed, Direction direction, int health,
	       int* windowBuffer, UPoint windowSize, std::vector<std::shared_ptr<BaseObj>>* allPawns,
	       std::shared_ptr<EventSystem> events);

	~Bullet() override;

	void Subscribe();
	void Unsubscribe() const;
	void Reset(const Rectangle& rect, int damage, double aoeRadius, int color, float speed, Direction direction,
	           int health);

	void Move(float deltaTime) override;
};
