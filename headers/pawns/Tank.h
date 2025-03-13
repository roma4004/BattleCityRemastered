#pragma once

#include "Pawn.h"
#include "../BulletPool.h"
#include "../bonuses/BonusStatus.h"
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

	std::shared_ptr<IShootable> _shootingBeh{nullptr};

protected:
	GameMode _gameMode{Demo};
	double _bulletDamageRadius{12.f};

	int _tier{0};
	std::chrono::milliseconds _fireCooldown{std::chrono::seconds{1}};
	mutable std::chrono::time_point<std::chrono::system_clock> _lastTimeFire;

	std::string _fraction;

	// bonuses
	BonusStatus _timer{}; //TODO: fix this for destroying tank, they respawn with false, need reuse instead of recreating, need pool objects for tanks
	BonusStatus _helmet{};

	void Shot() const;

	inline void SetPixel(size_t x, size_t y, int color) const;
	void DrawHealthBar() const override;

public:
	Tank(const ObjRectangle& rect, int color, int health, std::shared_ptr<Window> window, Direction direction,
	     float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects, std::shared_ptr<EventSystem> events,
	     std::unique_ptr<IMoveBeh> moveBeh, std::shared_ptr<IShootable> shootingBeh, std::string name,
	     std::string fraction, GameMode gameMode, int id);

	~Tank() override;

	virtual void Subscribe();
	virtual void Unsubscribe() const;

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

	[[nodiscard]] std::chrono::milliseconds GetFireCooldown() const;
	void SetFireCooldown(std::chrono::milliseconds fireCooldown);
};
