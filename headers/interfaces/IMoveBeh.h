#pragma once

#include <memory>
#include <vector>

class BaseObj;

class IMoveBeh
{
	[[nodiscard]] virtual std::vector<std::weak_ptr<BaseObj>> IsCanMove(float deltaTime) const = 0;

protected:
	virtual void MoveLeft(float deltaTime) const = 0;
	virtual void MoveRight(float deltaTime) const = 0;
	virtual void MoveUp(float deltaTime) const = 0;
	virtual void MoveDown(float deltaTime) const = 0;

public:
	virtual ~IMoveBeh() = default;
	virtual void Move(float deltaTime) const = 0;
};
