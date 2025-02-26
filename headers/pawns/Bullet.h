#pragma once

#include "Pawn.h"

#include <memory>
#include <string>

struct UPoint;
class BaseObj;
class EventSystem;

class Bullet final : public Pawn
{
	std::string _name;
	std::string _author;
	std::string _fraction;
	double _bulletDamageRadius{12.f};
	int _damage{0};
	int _id{0};
	bool _isNetworkControlled{false};

	void Subscribe();
	void Unsubscribe() const;

	void TickUpdate(float deltaTime) override;

public:
	Bullet(const ObjRectangle& rect, int damage, double aoeRadius, int color, int health,
	       std::shared_ptr<int[]> windowBuffer,
	       UPoint windowSize, Direction direction, float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	       const std::shared_ptr<EventSystem>& events, std::string author, std::string fraction, int bulletId,
	       bool isNetworkControlled);

	~Bullet() override;

	void Reset(const ObjRectangle& rect, int damage, double aoeRadius, int color, float speed, Direction direction,
	           int health, std::string author, std::string fraction, bool isNetworkControlled);

	void Disable() const;
	void Enable();

	[[nodiscard]] int GetDamage() const;

	[[nodiscard]] double GetBulletDamageRadius() const;

	[[nodiscard]] std::string GetName() const;

	[[nodiscard]] std::string GetAuthor() const;

	[[nodiscard]] std::string GetFraction() const;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;
	void TakeDamage(int damage) override;

	[[nodiscard]] int GetId() const;
};
