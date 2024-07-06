#pragma once

#include "MoveLikeBulletBeh.h"
#include "Pawn.h"

#include <string>

struct UPoint;
class BaseObj;
class EventSystem;

class Bullet : public Pawn
{
	std::string _name;
	std::string _author;
	std::string _fraction;
	int _damage{0};
	double _bulletDamageAreaRadius{12.f};

	void Subscribe();
	void Unsubscribe() const;

	void TickUpdate(float deltaTime) override;

public:
	Bullet(const Rectangle& rect, int damage, double aoeRadius, int color, int health, int* windowBuffer,
	       UPoint windowSize, Direction direction, float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	       const std::shared_ptr<EventSystem>& events, std::string author, std::string fraction);

	~Bullet() override;

	void Reset(const Rectangle& rect, int damage, double aoeRadius, int color, float speed, Direction direction,
	           int health, std::string author, std::string fraction);

	void Disable() const;
	void Enable();

	[[nodiscard]] int GetDamage() const;

	[[nodiscard]] double GetBulletDamageAreaRadius() const;

	[[nodiscard]] std::string GetAuthor() const;

	[[nodiscard]] std::string GetFraction() const;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;
};
