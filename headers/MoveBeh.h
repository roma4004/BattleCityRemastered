#pragma once

#include "Direction.h"

#include <list>
#include <memory>

class BaseObj;

class MoveBeh
{
public:
	virtual ~MoveBeh() = default;

	virtual void MoveLeft(float deltaTime) const = 0;
	virtual void MoveRight(float deltaTime) const = 0;
	virtual void MoveUp(float deltaTime) const = 0;
	virtual void MoveDown(float deltaTime) const = 0;

	[[nodiscard]] virtual Direction GetDirection() const = 0;
	virtual void SetDirection(Direction direction) = 0;
	[[nodiscard]] virtual float GetSpeed() const = 0;

private:
	[[nodiscard]] virtual std::list<std::weak_ptr<BaseObj>> IsCanMove(float deltaTime) const = 0;
};
