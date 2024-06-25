#pragma once

#include "../headers/Bullet.h"

#include <utility>

class MockBullet final : public Bullet
{
public:
	MockBullet(const Rectangle& rect, int damage, double aoeRadius, const int color, const float speed,
	           const Direction direction, const int health, int* windowBuffer, const UPoint windowSize,
	           std::vector<std::shared_ptr<BaseObj>>* allPawns, std::shared_ptr<EventSystem> events)
		: Bullet(rect, damage, aoeRadius, color, speed, direction, health, windowBuffer, windowSize, allPawns,
		         std::move(events)) {}

	~MockBullet() override = default;

	void SetDirection(Direction direction) const { _moveBeh->SetDirection(direction); }
};
