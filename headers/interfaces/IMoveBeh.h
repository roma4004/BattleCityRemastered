#pragma once

#include <memory>
#include <vector>

class BaseObj;

class IMoveBeh
{
	[[nodiscard]] virtual std::vector<std::shared_ptr<BaseObj>> IsCanMove(float deltaTime) const = 0;

protected:
	[[nodiscard]] virtual bool MoveLeft(float deltaTime) const = 0;
	[[nodiscard]] virtual bool MoveRight(float deltaTime) const = 0;
	[[nodiscard]] virtual bool MoveUp(float deltaTime) const = 0;
	[[nodiscard]] virtual bool MoveDown(float deltaTime) const = 0;

public:
	virtual ~IMoveBeh() = default;

	[[nodiscard]] virtual bool Move(float deltaTime) const = 0;
};
