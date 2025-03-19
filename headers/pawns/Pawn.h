#pragma once

#include "../BaseObj.h"
#include "../interfaces/ITickUpdatable.h"

#include <memory>
#include <vector>

enum Direction : char8_t;
enum GameMode : char8_t;
struct ObjRectangle;
struct Window;
struct UPoint;
class IMoveBeh;
class EventSystem;

class Pawn : public BaseObj, public ITickUpdatable
{
	virtual void UnsubscribeAsHost() const;
	virtual void UnsubscribeAsClient() const;

	virtual void SubscribeAsHost();
	virtual void SubscribeAsClient();

	void Draw() const override;

protected:
	Direction _direction{};
	GameMode _gameMode{};
	float _speed{0.f};
	int _tier{1};

	std::unique_ptr<IMoveBeh> _moveBeh{nullptr};
	std::shared_ptr<Window> _window{nullptr};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};

	virtual void Subscribe();
	virtual void Unsubscribe() const;

	//TODO: implement collision detection through quadtree
	void TickUpdate(float deltaTime) override = 0;

public:
	Pawn(const ObjRectangle& rect, int color, int health, std::shared_ptr<Window> window, Direction dir, float speed,
	     std::vector<std::shared_ptr<BaseObj>>* allObjects, std::shared_ptr<EventSystem> events,
	     std::unique_ptr<IMoveBeh> moveBeh, int id, std::string name, std::string fraction, int tier,
	     GameMode gameMode);

	~Pawn() override;

	[[nodiscard]] UPoint GetWindowSize() const;

	[[nodiscard]] Direction GetDirection() const;
	void SetDirection(Direction dir);

	[[nodiscard]] float GetSpeed() const;
	void SetSpeed(float speed);
};
