#pragma once

#include "Point.h"
#include "../BaseObj.h"
#include "interfaces/ITickUpdatable.h"

struct PawnProperty;
enum Direction : char8_t;
enum GameMode : char8_t;
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

	void Draw(const BaseObj* obj) const override;

protected:
	Direction _dir{};
	GameMode _gameMode{};
	float _speed{0.f};
	int _tier{1};
	UPoint _windowSize{};

	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<IMoveBeh> _moveBeh{nullptr};

	virtual void Subscribe();
	virtual void Unsubscribe() const;

	//TODO: implement collision detection through quadtree
	void TickUpdate(float deltaTime) override = 0;

public:
	int animationId{0};
	int animationIdLimit{1};
	int animationFrameId{0};

	Pawn(PawnProperty pawnProperty, std::unique_ptr<IMoveBeh> moveBeh);

	~Pawn() override;

	void SetHealth(int health) override;
	void TakeDamage(int damage) override;
	void UpdateAnimationFrame();

	[[nodiscard]] UPoint GetWindowSize() const;

	[[nodiscard]] Direction GetDirection() const;
	void SetDirection(Direction dir);

	[[nodiscard]] float GetSpeed() const;
	void SetSpeed(float speed);
};
