#pragma once

#include "Pawn.h"

#include <chrono>

struct UPoint;

class Tank : public Pawn
{
	int _damage{15};

	float _bulletWidth{6.f};

	float _bulletHeight{6.f};

	float _bulletSpeed{300.f};

protected:
	std::chrono::time_point<std::chrono::system_clock> lastTimeFire;
	int fireCooldown{2};

public:
	Tank(const Rectangle& rect, int color, int health, int* windowBuffer, UPoint windowSize,
	     std::vector<std::shared_ptr<BaseObj>>* allPawns, std::shared_ptr<EventSystem> events,
	     std::shared_ptr<MoveBeh> moveBeh);

	~Tank() override = default;

	void Shot();

	[[nodiscard]] float GetBulletWidth() const;
	void SetBulletWidth(float bulletWidth);

	[[nodiscard]] float GetBulletHeight() const;
	void SetBulletHeight(float bulletHeight);

	[[nodiscard]] float GetBulletSpeed() const;
	void SetBulletSpeed(float bulletSpeed);

	bool IsReloadFinish() const;
};
