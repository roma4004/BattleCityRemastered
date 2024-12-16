#pragma once

#include "Pawn.h"
#include "../BulletPool.h"
#include "../interfaces/IHealthBar.h"
#include "../interfaces/IShootable.h"

#include <chrono>

struct UPoint;

class Tank : public Pawn, public IHealthBar
{
	int _bulletDamage{15};

	float _bulletWidth{6.f};

	float _bulletHeight{6.f};

	float _bulletSpeed{300.f};//TODO: move outside this class to bullet calibre stats class and DI into constructor

	std::shared_ptr<IShootable> _shootingBeh;
	int _tankId;

protected:
	double _bulletDamageAreaRadius{12.f};

	std::chrono::time_point<std::chrono::system_clock> _lastTimeFire;

	int _tier{0};
	int _fireCooldownMs{1000};//1 sec

	std::string _name;
	std::string _fraction;

	// bonuses
	bool _isActiveTimer{false};
	//TODO: fix this for destroying tank, they respawn with false, need reuse instead of recreating, need pool objects for tanks
	std::chrono::system_clock::time_point _activateTimeTimer;
	int _cooldownTimer{0};

	bool _isActiveHelmet{false};
	std::chrono::system_clock::time_point _activateTimeHelmet;
	int _cooldownHelmet{0};

	void Shot() const;

	void DrawHealthBar() const override;

public:
	Tank(const ObjRectangle& rect, int color, int health, int* windowBuffer, UPoint windowSize,
	     Direction direction, float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	     const std::shared_ptr<EventSystem>& events, std::shared_ptr<IMoveBeh> moveBeh,
	     std::shared_ptr<IShootable> shootingBeh, const std::string& name, std::string fraction, int tankId);

	~Tank() override = default;

	[[nodiscard]] std::string GetName() const;
	[[nodiscard]] std::string GetFraction() const;

	void TakeDamage(int damage) override;

	[[nodiscard]] int GetTankTier() const;

	[[nodiscard]] float GetBulletWidth() const;
	void SetBulletWidth(float bulletWidth);

	[[nodiscard]] float GetBulletHeight() const;
	void SetBulletHeight(float bulletHeight);

	[[nodiscard]] float GetBulletSpeed() const;
	void SetBulletSpeed(float bulletSpeed);

	[[nodiscard]] int GetBulletDamage() const;
	void SetBulletDamage(int bulletDamage);

	[[nodiscard]] double GetBulletDamageAreaRadius() const;
	void SetBulletDamageAreaRadius(double bulletDamageAreaRadius);

	[[nodiscard]] int GetFireCooldownMs() const;
	void SetFireCooldownMs(int fireCooldown);

	[[nodiscard]] int GetTankId() const;
};
