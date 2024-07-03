#pragma once

#include "BulletPool.h"
#include "Pawn.h"
#include "Interfaces/IHealthBar.h"

#include <chrono>

struct UPoint;

class Tank : public Pawn, public IHealthBar
{
	int _damage{15};

	float _bulletWidth{6.f};

	float _bulletHeight{6.f};

	float _bulletSpeed{300.f};//TODO: move outside this class to bullet calibre stats class and DI into constructor

protected:
	double _bulletDamageAreaRadius{12.f};

	std::chrono::time_point<std::chrono::system_clock> lastTimeFire;

	int fireCooldown{1};

	std::shared_ptr<BulletPool> _bulletPool;

	std::string _name;
	std::string _fraction;

public:
	Tank(const Rectangle& rect, int color, int health, int* windowBuffer, UPoint windowSize,
	     Direction direction, float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	     std::shared_ptr<EventSystem> events, std::shared_ptr<IMoveBeh> moveBeh, std::shared_ptr<BulletPool> bulletPool,
	     std::string name, std::string fraction);
	Rectangle GetBulletStartRect() const;

	~Tank() override = default;

	void Shot() const;

	[[nodiscard]] float GetBulletWidth() const;
	void SetBulletWidth(float bulletWidth);

	[[nodiscard]] float GetBulletHeight() const;
	void SetBulletHeight(float bulletHeight);

	[[nodiscard]] float GetBulletSpeed() const;
	void SetBulletSpeed(float bulletSpeed);

	[[nodiscard]] bool IsReloadFinish() const;

	void DrawHealthBar() const override;
};
