#pragma once

#include "Direction.h"

#include <list>
#include <memory>

class BaseObj;

class IMoveBeh
{
public:
	virtual ~IMoveBeh() = default;

	virtual void Move(float deltaTime) const = 0;
	virtual void MoveLeft(float deltaTime) const = 0;
	virtual void MoveRight(float deltaTime) const = 0;
	virtual void MoveUp(float deltaTime) const = 0;
	virtual void MoveDown(float deltaTime) const = 0;

private:
	[[nodiscard]] virtual std::list<std::weak_ptr<BaseObj>> IsCanMove(float deltaTime) const = 0;
};
