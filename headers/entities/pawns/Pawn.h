#pragma once

#include "../BaseObj.h"
#include "Point.h"
#include "interfaces/ITickUpdatable.h"

struct PawnProperty;
enum class Direction : char8_t;
enum class GameMode : char8_t;
struct ObjRectangle;
class IMoveBeh;
class EventSystem;

class Pawn : public BaseObj, public ITickUpdatable
{
	using buuid = boost::uuids::uuid;

	virtual void UnsubscribeAsHost() const;
	virtual void UnsubscribeAsClient() const;

	virtual void SubscribeAsHost();
	virtual void SubscribeAsClient();

protected:
	float _speed{};
	std::string _uuidStr{};
	int _tier{1};
	UPoint _windowSize{};
	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<IMoveBeh> _moveBeh{nullptr};
	Direction _dir{};
	GameMode _gameMode{};

	virtual void Subscribe();
	virtual void Unsubscribe() const;

	void SubscribeTickUpdate();
	void UnsubscribeTickUpdate() const;

	//TODO: implement collision detection through quadtree
	void TickUpdate(double deltaTime) override = 0;

public:
	Pawn(PawnProperty pawnProperty);

	~Pawn() override;

	void TakeDamage(int damage) override;

	[[nodiscard]] UPoint GetWindowSize() const;

	[[nodiscard]] Direction GetDirection() const;
	void SetDirection(Direction dir);

	[[nodiscard]] float GetSpeed() const;
	void SetSpeed(float speed);

	[[nodiscard]] virtual bool Move(double deltaTime);
	void OnClientChangePos(FPoint newPos, Direction dir, const buuid& uuid);
};
