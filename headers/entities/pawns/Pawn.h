#pragma once

#include "../BaseObj.h"
#include "interfaces/ITickUpdatable.h"

enum class Direction : char8_t;
enum class GameMode : char8_t;
struct PawnProperty;
struct ObjRectangle;
class IMoveBeh;
class EventSystem;
class GameConfig;

class Pawn : public BaseObj, public ITickUpdatable
{
	using buuid = boost::uuids::uuid;

public:
	Pawn(PawnProperty pawnProperty, GameConfig& gameConfig);

	~Pawn() override;

	//BaseObj overrides
	void TakeDamage(unsigned int damage, const std::string& damageAuthor, const std::string& damageFraction) override;

	[[nodiscard]] Direction GetDirection() const;
	void SetDirection(Direction dir);

	[[nodiscard]] float GetSpeed() const;
	void SetSpeed(float speed);

protected:
	float _speed{};
	std::string _uuidStr{};
	unsigned short _tier{1u};
	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<IMoveBeh> _moveBeh{nullptr};
	Direction _dir{};
	GameMode _gameMode{};
	GameConfig& _gameConfig;

	virtual void Subscribe();
	virtual void Unsubscribe() const;

	void SubscribeTickUpdate();
	void UnsubscribeTickUpdate() const;

	//TODO: implement collision detection through quadtree
	void TickUpdate(double deltaTime) override = 0;

private:
	virtual void SubscribeAsHost();
	virtual void SubscribeAsClient();
};
