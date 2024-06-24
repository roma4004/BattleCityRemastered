#pragma once

#include "Pawn.h"

#include <chrono>

struct UPoint;

class Tank : public Pawn
{
	int _damage{15};

	float _bulletWidth{6.f};

	float _bulletHeight{6.f};

	float _bulletSpeed{300.f}; //TODO: move outside this class to bullet calibre stats class and DI into constructor

protected:
	double _bulletDamageAreaRadius{12.f};

	std::chrono::time_point<std::chrono::system_clock> lastTimeFire;

	int fireCooldown{1};

public:
	Tank(const Rectangle& rect, int color, int health, int* windowBuffer, UPoint windowSize,
	     std::vector<std::shared_ptr<BaseObj>>* allPawns, std::shared_ptr<EventSystem> events,
	     std::shared_ptr<IMoveBeh> moveBeh);

	~Tank() override = default;

	void Shot();

	[[nodiscard]] float GetBulletWidth() const;
	void SetBulletWidth(float bulletWidth);

	[[nodiscard]] float GetBulletHeight() const;
	void SetBulletHeight(float bulletHeight);

	[[nodiscard]] float GetBulletSpeed() const;
	void SetBulletSpeed(float bulletSpeed);

	[[nodiscard]] bool IsReloadFinish() const;
};
