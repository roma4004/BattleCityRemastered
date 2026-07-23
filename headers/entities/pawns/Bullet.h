#pragma once

#include "Pawn.h"
#include "entities/BulletCalibre.h"
#include "interfaces/IDrawable.h"
#include <boost/uuid/uuid.hpp>
#include <string>

struct BulletResetProperty;
struct UPoint;
class EventSystem;
class BulletPool;
class ShootingBeh;
class GameConfig;

class Bullet final : public Pawn, public IDrawable
{
	friend BulletPool;
	friend ShootingBeh;

	using buuid = boost::uuids::uuid;

	std::string _author{};
	BulletCalibre _calibre{};

	void SubscribeAsClient() override;
	void Enable();
	void Disable() const;
	void Reset(BulletResetProperty resetProperty);

protected:
	void Subscribe() override;
	void Unsubscribe() const override;
	void Draw() const override;
	void TickUpdate(double deltaTime) override;

public:
	Bullet(PawnProperty pawnProperty, GameConfig& gameConfig, const BulletCalibre& calibre = {},
		   std::string author = "", bool enableByDefault = false);

	~Bullet() override;

	[[nodiscard]] int GetDamage() const;

	[[nodiscard]] double GetDamageRadius() const;

	[[nodiscard]] std::string GetAuthor() const;

	[[nodiscard]] buuid GetUuid() const override;
	[[nodiscard]] const std::string& GetUuidStr() const;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

	//BaseObj overrides
	void TakeDamage(int damage, const std::string& damageAuthor, const std::string& damageFraction) override;

	[[nodiscard]] unsigned int GetTier() const;

	void DealDamage(const std::vector<std::shared_ptr<BaseObj>>& objectList);
	void OnClientChangePos(FPoint newPos, Direction dir, const buuid& uuid);
};
