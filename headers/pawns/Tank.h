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
	std::chrono::milliseconds _fireCooldown{std::chrono::seconds{1}};
	mutable std::chrono::time_point<std::chrono::system_clock> _lastTimeFire;

	// bonuses
	BonusStatus _timer{};
	//TODO: fix this for destroying tank, they respawn with false, need reuse instead of recreating, need pool objects for tanks
	BonusStatus _helmet{};

	void Shot() const;

	inline void SetPixel(size_t x, size_t y, int color) const;
	void DrawHealthBar() const override;

	void OnBonusTimer(const std::string& fraction, std::chrono::milliseconds duration);
	void OnBonusHelmet(const std::string& author, const std::string& fraction, std::chrono::milliseconds duration);
	void OnBonusGrenade(const std::string& author, const std::string& fraction);
	void OnBonusStar(const std::string& author, const std::string& fraction);

public:
	Tank(const ObjRectangle& rect, int color, int health, std::shared_ptr<Window> window, Direction direction,
	     float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects, std::shared_ptr<EventSystem> events,
	     std::unique_ptr<IMoveBeh> moveBeh, std::shared_ptr<IShootable> shootingBeh, std::string name,
	     std::string fraction, GameMode gameMode, int id, int tier);

	~Tank() override;

	virtual void Subscribe();
	void SubscribeAsHost();
	void SubscribeAsClient();
	void SubscribeBonus();

	virtual void Unsubscribe() const;
	void UnsubscribeAsHost() const;
	void UnsubscribeAsClient() const;
	void UnsubscribeBonus() const;

	void TakeDamage(int damage) override;

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

	[[nodiscard]] std::chrono::milliseconds GetFireCooldown() const;
	void SetFireCooldown(std::chrono::milliseconds fireCooldown);
};
