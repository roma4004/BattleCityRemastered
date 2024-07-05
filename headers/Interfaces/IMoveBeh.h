#pragma once

#include <list>
#include <memory>

class BaseObj;

class IMoveBeh
{
	[[nodiscard]] virtual std::list<std::weak_ptr<BaseObj>> IsCanMove(float deltaTime) const = 0;

protected:
	virtual ~IMoveBeh() = default;

	virtual void MoveLeft(float deltaTime) const = 0;
	virtual void MoveRight(float deltaTime) const = 0;
	virtual void MoveUp(float deltaTime) const = 0;
	virtual void MoveDown(float deltaTime) const = 0;

public:
	virtual void Move(float deltaTime) const = 0;
};
