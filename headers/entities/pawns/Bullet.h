#pragma once

#include "Pawn.h"
#include "interfaces/IDrawable.h"
#include <boost/uuid/uuid.hpp>
#include <string>

struct BulletResetProperty;
struct UPoint;
class EventSystem;
class BulletPool;
class ShootingBeh;

class Bullet final : public Pawn, public IDrawable
{
	friend BulletPool;
	friend ShootingBeh;

	using buuid = boost::uuids::uuid;

	std::string _author{};
	double _bulletDamageRadius{14.f};
	int _damage{0};
	std::vector<std::shared_ptr<BaseObj>> _bulletTargets{};

	void Subscribe() override;
	void SubscribeAsClient() override;

	void Unsubscribe() const override;

	void Enable();
	void Disable() const;

	void Draw() const override;
	void TickUpdate(double deltaTime) override;

	void Reset(BulletResetProperty resetProperty);

public:
	explicit Bullet(PawnProperty pawnProperty);
	Bullet(PawnProperty pawnProperty, int damage, double aoeRadius, std::string author, bool enableByDefault = false);

	~Bullet() override;

	[[nodiscard]] int GetDamage() const;

	[[nodiscard]] double GetBulletDamageRadius() const;

	[[nodiscard]] std::string GetAuthor() const;

	[[nodiscard]] buuid GetUuid() const override;
	[[nodiscard]] const std::string& GetUuidStr() const;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

	void TakeDamage(int damage) override;

	[[nodiscard]] int GetTier() const;

	void DealDamage(const std::vector<std::shared_ptr<BaseObj>>& objectList);
};
