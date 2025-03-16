#pragma once

#include "../BaseObj.h"
#include "../Direction.h"
#include "../Point.h"
#include "../application/Window.h"
#include "../interfaces/IMoveBeh.h"
#include "../interfaces/ITickUpdatable.h"

#include <memory>
#include <vector>

struct ObjRectangle;
class EventSystem;

class Pawn : public BaseObj, public ITickUpdatable
{
protected:
	Direction _direction{UP};
	float _speed{0.f};
	int _tier{1};

	std::unique_ptr<IMoveBeh> _moveBeh{nullptr};
	std::shared_ptr<Window> _window{nullptr};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};

	void Draw() const override;

	//TODO: implement collision detection through quadtree
	void TickUpdate(float deltaTime) override = 0;

public:
	Pawn(const ObjRectangle& rect, int color, int health, std::shared_ptr<Window> window, Direction direction,
	     float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects, std::shared_ptr<EventSystem> events,
	     std::unique_ptr<IMoveBeh> moveBeh, int id, std::string name, std::string fraction, int tier);

	~Pawn() override;

	[[nodiscard]] UPoint GetWindowSize() const;

	[[nodiscard]] Direction GetDirection() const;
	void SetDirection(Direction direction);

	[[nodiscard]] float GetSpeed() const;
	void SetSpeed(float speed);
};
