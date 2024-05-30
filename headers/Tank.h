#pragma once

#include "Pawn.h"

struct UPoint;

class Tank : public Pawn
{
public:
	Tank(const Rectangle& rect, int color, float speed, int health, int* windowBuffer, UPoint windowSize,
	     std::vector<std::shared_ptr<BaseObj>>* allPawns, std::shared_ptr<EventSystem> events);

	~Tank() override = default;

	void Shot();//TODO: create shoot beh and remove this 

	[[nodiscard]] float GetBulletWidth() const;
	void SetBulletWidth(float bulletWidth);

	[[nodiscard]] float GetBulletHeight() const;
	void SetBulletHeight(float bulletHeight);

	[[nodiscard]] float GetBulletSpeed() const;
	void SetBulletSpeed(float bulletSpeed);

private:
	int _damage{15};

	float _bulletWidth{6.f};

	float _bulletHeight{6.f};

	float _bulletSpeed{300.f};
};