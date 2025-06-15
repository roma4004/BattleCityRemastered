#pragma once

#include "Pawn.h"
#include "../BonusEffectProperty.h"
#include "../Point.h"
#include "../bonuses/BonusStatus.h"
#include "../interfaces/IHealthBar.h"

#include <chrono>

struct UPoint;
class IShootable;

class Tank : public Pawn, public IHealthBar
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

	int _bulletDamage{15};
	float _bulletSpeed{300.f};//TODO: move outside this class to bullet calibre stats class and DI into constructor

	std::shared_ptr<IShootable> _shootingBeh{nullptr};

	void Subscribe() override;
	void SubscribeAsClient() override;
	void SubscribeBonus();

	void Unsubscribe() const override;
	void UnsubscribeAsClient() const override;
	void UnsubscribeBonus() const;

	void DrawHealthBar(const BaseObj* obj) const override;
	void OnBonusTimer(const std::string& fraction, bool isActive);
	void OnBonusHelmet(const std::string& name, bool isActive);

	void OnBonusGrenade(const std::string& author, const std::string& fraction);
	void OnBonusStar(const std::string& author, const std::string& fraction);

protected:
	FPoint _bulletSize{9.f, 9.f};
	double _bulletDamageRadius{18.f};
	milliseconds _fireCooldown{std::chrono::seconds{1}};
	mutable std::chrono::time_point<std::chrono::system_clock> _lastTimeFire;

	// bonuses
	BonusEffectProperty _effects{};
	//in progress TODO: fix this for destroying tank, they respawn with false, need reuse instead of recreating, need pool objects for tanks
	BonusStatus _helmet{};

	void Shot(buuid withUuid = {}) const;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

	void TickUpdate(float deltaTime) override = 0;

	void TakeDamage(int damage) override;

public:
	Tank(PawnProperty pawnProperty, std::unique_ptr<IMoveBeh> moveBeh, std::shared_ptr<IShootable> shootingBeh,
	     BonusEffectProperty effects);

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
