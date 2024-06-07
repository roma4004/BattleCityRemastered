#pragma once

#include "BaseObj.h"
#include "IMovable.h"
#include "MoveBeh.h"
#include "PlayerKeys.h"
#include "Point.h"
#include "Rectangle.h"

#include <memory>
#include <vector>

class EventSystem;

class Pawn : public BaseObj, public IMovable
{
protected:
	std::shared_ptr<MoveBeh> _moveBeh;

	UPoint _windowSize{0, 0};

	int* _windowBuffer{nullptr};

	std::shared_ptr<EventSystem> _events;

	std::vector<std::shared_ptr<BaseObj>>* _allPawns{nullptr};

public:
	Pawn(const Rectangle& rect, int color, int health, int* windowBuffer, UPoint windowSize,
	     std::vector<std::shared_ptr<BaseObj>>* allPawns, std::shared_ptr<EventSystem> events,
	     std::shared_ptr<MoveBeh> moveBeh);

	~Pawn() override;

	void SetPixel(size_t x, size_t y, int color) const;

	void Move(float deltaTime) override;

	void Draw() const override;

	void TickUpdate(float deltaTime) override;

	PlayerKeys keyboardButtons{}; // Move out from this
};
