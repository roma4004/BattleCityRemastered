#pragma once

#include "Pawn.h"
#include "../Point.h"
#include "../bonuses/BonusStatus.h"
#include "../interfaces/IHealthBar.h"

#include <chrono>

struct UPoint;
class IShootable;

class Tank : public Pawn, public IHealthBar
{
	int _bulletDamage{15};
	float _bulletSpeed{300.f};//TODO: move outside this class to bullet calibre stats class and DI into constructor

	std::shared_ptr<IShootable> _shootingBeh{nullptr};

	void Subscribe() override;
	void SubscribeAsClient() override;
	void SubscribeBonus();

	void Unsubscribe() const override;
	void UnsubscribeAsClient() const override;
	void UnsubscribeBonus() const;

	void DrawHealthBar() const override;

	inline void SetPixel(size_t x, size_t y, int color) const;

	void OnBonusTimer(const std::string& fraction, std::chrono::milliseconds duration);
	void OnBonusHelmet(const std::string& author, const std::string& fraction, std::chrono::milliseconds duration);
	void OnBonusGrenade(const std::string& author, const std::string& fraction);
	void OnBonusStar(const std::string& author, const std::string& fraction);

protected:
	FPoint _bulletSize{6.f, 6.f};
	double _bulletDamageRadius{12.f};
	std::chrono::milliseconds _fireCooldown{std::chrono::seconds{1}};
	mutable std::chrono::time_point<std::chrono::system_clock> _lastTimeFire;

	// bonuses
	BonusStatus _timer{};
	//TODO: fix this for destroying tank, they respawn with false, need reuse instead of recreating, need pool objects for tanks
	BonusStatus _helmet{};

	void Shot() const;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

	void TickUpdate(float deltaTime) override;

	void TakeDamage(int damage) override;

public:
	Tank(const ObjRectangle& rect, int color, int health, std::shared_ptr<Window> window, Direction dir, float speed,
	     std::vector<std::shared_ptr<BaseObj>>* allObjects, std::shared_ptr<EventSystem> events,
	     std::unique_ptr<IMoveBeh> moveBeh, std::shared_ptr<IShootable> shootingBeh, std::string name,
	     std::string fraction, GameMode gameMode, int id, int tier);

	~Tank() override;

	[[nodiscard]] int GetTier() const;

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
};
