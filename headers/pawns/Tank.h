#pragma once

#include "Pawn.h"
#include "../BulletPool.h"
#include "../interfaces/IHealthBar.h"

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

	std::chrono::time_point<std::chrono::system_clock> _lastTimeFire;

	int fireCooldown{1};

	std::shared_ptr<BulletPool> _bulletPool;

	std::string _name;
	std::string _fraction;

	// bonuses
	bool _isActiveTimer{false}; //TODO: fix this for destroying tank, they respawn with false, need reuse instead of recreating
	std::chrono::system_clock::time_point _activateTimeTimer;
	int _cooldownTimer{0};

	bool _isActiveHelmet{false};
	std::chrono::system_clock::time_point _activateTimeHelmet;
	int _cooldownHelmet{0};

	void Shot() const;

	[[nodiscard]] float GetBulletWidth() const;
	void SetBulletWidth(float bulletWidth);

	[[nodiscard]] float GetBulletHeight() const;
	void SetBulletHeight(float bulletHeight);

	[[nodiscard]] float GetBulletSpeed() const;
	void SetBulletSpeed(float bulletSpeed);

	[[nodiscard]] bool IsReloadFinish() const;

	void DrawHealthBar() const override;

public:
	Tank(const Rectangle& rect, int color, int health, int* windowBuffer, UPoint windowSize,
	     Direction direction, float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	     std::shared_ptr<EventSystem> events, std::shared_ptr<IMoveBeh> moveBeh, std::shared_ptr<BulletPool> bulletPool,
	     std::string name, std::string fraction);

	~Tank() override = default;

	[[nodiscard]] Rectangle GetBulletStartRect() const;

	[[nodiscard]] std::string GetName() const { return _name; }
	[[nodiscard]] std::string GetFraction() const { return _fraction; }

	void TakeDamage(int damage) override;
};
