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
	int _id;
	float _bulletWidth{6.f};
	float _bulletHeight{6.f};
	float _bulletSpeed{300.f};//TODO: move outside this class to bullet calibre stats class and DI into constructor

	std::shared_ptr<IShootable> _shootingBeh{nullptr};

protected:
	GameMode _gameMode{Demo};
	double _bulletDamageRadius{12.f};

	mutable std::chrono::time_point<std::chrono::system_clock> _lastTimeFire;

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
	Tank(const ObjRectangle& rect, int color, int health, std::shared_ptr<int[]> windowBuffer, UPoint windowSize,
	     Direction direction, float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	     std::shared_ptr<EventSystem> events, std::unique_ptr<IMoveBeh> moveBeh,
	     std::shared_ptr<IShootable> shootingBeh, std::string name, std::string fraction, GameMode gameMode,
	     int id);

	~Tank() override;

	virtual void Subscribe();
	virtual void Unsubscribe() const;

	[[nodiscard]] int GetId() const;
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

	[[nodiscard]] double GetBulletDamageRadius() const;
	void SetBulletDamageRadius(double bulletDamageRadius);

	[[nodiscard]] int GetFireCooldownMs() const;
	void SetFireCooldownMs(int fireCooldown);
};
