#pragma once

#include "Pawn.h"
#include "../BonusEffectProperty.h"
#include "interfaces/IHealthBar.h"

struct UPoint;
class PlayerTest;
class IShootable;
class SpawnDelayManager;
class BulletPool;

class Tank : public Pawn, public IHealthBar
{
	friend class SpawnDelayManager;

	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

	int _bulletDamage{15};
	float _bulletSpeed{300.f};//TODO: move outside this class to bullet caliber stats class and DI into constructor
	std::vector<std::shared_ptr<BaseObj>> _touchedObstacles;

	std::shared_ptr<IShootable> _shootingBeh{nullptr};

	void Subscribe() override;
	void SubscribeAsClient() override;
	void SubscribeBonus();

	void Disable() const;
	void Enable();

	void Unsubscribe() const override;
	void UnsubscribeAsClient() const override;
	void UnsubscribeBonus() const;

	void DrawHealthBar(const BaseObj* obj) const override;
	void OnBonusTimer(const std::string& fraction, bool isActive);
	void OnBonusHelmet(const std::string& name, bool isActive);

	void OnBonusGrenade(const std::string& author, const std::string& fraction);
	void OnBonusStar(const std::string& author, const std::string& fraction);
	void OnBonusCaliber(const std::string& author, const std::string& fraction);

protected:
	FPoint _bulletSize{.x = 9.f, .y = 9.f};
	double _bulletDamageRadius{18.f};
	milliseconds _fireCooldown{std::chrono::seconds{1}};
	mutable std::chrono::time_point<std::chrono::system_clock> _lastTimeFire{};

	// bonuses
	BonusEffectProperty _effects{};
	//in progress TODO: fix this for destroying tank, they respawn with false, need reuse instead of recreating, need pool objects for tanks

	void Shot(buuid withUuid = {}) const;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

	void TickUpdate(float deltaTime) override = 0;

	void TakeDamage(int damage) override;

public:
	Tank(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool, BonusEffectProperty effects,
	     bool enableByDefault = false);

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
