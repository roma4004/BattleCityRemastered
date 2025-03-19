#pragma once

#include "Pawn.h"

#include <memory>
#include <string>

struct UPoint;
class EventSystem;

class Bullet final : public Pawn
{
	std::string _author;
	double _bulletDamageRadius{12.f};
	int _damage{0};

	void Subscribe() override;
	void SubscribeAsClient() override;

	void Unsubscribe() const override;
	void UnsubscribeAsClient() const override;

	void TickUpdate(float deltaTime) override;

public:
	Bullet(const ObjRectangle& rect, int damage, double aoeRadius, int color, int health,
	       std::shared_ptr<Window> window, Direction dir, float speed,
	       std::vector<std::shared_ptr<BaseObj>>* allObjects, const std::shared_ptr<EventSystem>& events,
	       std::string author, std::string fraction, GameMode gameMode, int id, int tier = 1);

	~Bullet() override;

	void Reset(const ObjRectangle& rect, int damage, double aoeRadius, int color, float speed, Direction dir,
	           int health, std::string author, std::string fraction, int tier);

	void Disable() const;
	void Enable();

	[[nodiscard]] int GetDamage() const;

	[[nodiscard]] double GetBulletDamageRadius() const;

	[[nodiscard]] std::string GetAuthor() const;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

	void TakeDamage(int damage) override;

	[[nodiscard]] int GetTier() const;
};
