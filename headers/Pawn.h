#pragma once

#include "BaseObj.h"
#include "Direction.h"
#include "Point.h"
#include "Rectangle.h"
#include "interfaces/IMoveBeh.h"
#include "interfaces/ITickUpdatable.h"

#include <memory>
#include <vector>

class EventSystem;

class Pawn : public BaseObj, public ITickUpdatable
{
protected:
	std::shared_ptr<IMoveBeh> _moveBeh;

	UPoint _windowSize{0, 0};

	int* _windowBuffer{nullptr};

	Direction _direction{UP};

	float _speed{0.f};

	std::shared_ptr<EventSystem> _events;

	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};

	void SetPixel(size_t x, size_t y, int color) const;

	void Draw() const override;

	//TODO: implement collision detection through quadtree
	void TickUpdate(float deltaTime) override = 0;

public:
	Pawn(const Rectangle& rect, int color, int health, int* windowBuffer, UPoint windowSize, Direction direction,
	     float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects, std::shared_ptr<EventSystem> events,
	     std::shared_ptr<IMoveBeh> moveBeh);

	~Pawn() override;

	[[nodiscard]] UPoint GetWindowSize() const;

	[[nodiscard]] Direction GetDirection() const { return _direction; }
	void SetDirection(const Direction direction) { _direction = direction; }

	[[nodiscard]] float GetSpeed() const { return _speed; }
};
