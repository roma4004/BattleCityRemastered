#pragma once

#include "Pawn.h"
#include <string>
#include <boost/uuid/uuid.hpp>

struct BulletResetProperty;
struct UPoint;
class EventSystem;

class Bullet final : public Pawn
{
	using buuid = boost::uuids::uuid;

	std::string _author{};
	double _bulletDamageRadius{14.f};
	std::string _uuidStr{};
	int _damage{0};

	void Subscribe() override;
	void SubscribeAsClient() override;

	void Unsubscribe() const override;
	void UnsubscribeAsClient() const override;

	void TickUpdate(float deltaTime) override;

public:
	explicit Bullet(PawnProperty pawnProperty);
	Bullet(PawnProperty pawnProperty, int damage, double aoeRadius, std::string author);

	~Bullet() override;

	void Reset(BulletResetProperty resetProperty);

	void Disable() const;
	void Enable();

	[[nodiscard]] int GetDamage() const;

	[[nodiscard]] double GetBulletDamageRadius() const;

	[[nodiscard]] std::string GetAuthor() const;

	[[nodiscard]] buuid GetUuid() const override;
	[[nodiscard]] const std::string& GetUuidStr() const;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

	void TakeDamage(int damage) override;

	[[nodiscard]] int GetTier() const;
};
