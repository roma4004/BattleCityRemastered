#pragma once

#include "Pawn.h"

#include <string>
#include <boost/uuid/uuid.hpp>

struct UPoint;
class EventSystem;

class Bullet final : public Pawn
{
	std::string _author;
	double _bulletDamageRadius{12.f};
	int _damage{0};
	std::string _uuidStr{};

	void Subscribe() override;
	void SubscribeAsClient() override;

	void Unsubscribe() const override;
	void UnsubscribeAsClient() const override;

	void TickUpdate(float deltaTime) override;

public:
	Bullet(PawnProperty pawnProperty, int damage, double aoeRadius, std::string author,
	       boost::uuids::uuid uuid = {}, const std::string& uuidStr = {});

	~Bullet() override;

	void Reset(const ObjRectangle& rect, int damage, double aoeRadius, int color, float speed, Direction dir,
	           int health, std::string author, std::string fraction, int tier, boost::uuids::uuid uuid = {});

	void Disable() const;
	void Enable();

	[[nodiscard]] int GetDamage() const;

	[[nodiscard]] double GetBulletDamageRadius() const;

	[[nodiscard]] std::string GetAuthor() const;

	[[nodiscard]] boost::uuids::uuid GetUuid() const override;
	[[nodiscard]] const std::string& GetUuidStr() const;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

	void TakeDamage(int damage) override;

	[[nodiscard]] int GetTier() const;
};
