#pragma once

#include <memory>
#include <vector>

class BaseObj;

class IMoveBeh
{
	[[nodiscard]] virtual std::vector<std::shared_ptr<BaseObj>> IsCanMove(float deltaTime) const = 0;

protected:
	virtual [[nodiscard]] bool MoveLeft(float deltaTime) const = 0;
	virtual [[nodiscard]] bool MoveRight(float deltaTime) const = 0;
	virtual [[nodiscard]] bool MoveUp(float deltaTime) const = 0;
	virtual [[nodiscard]] bool MoveDown(float deltaTime) const = 0;

public:
	virtual ~IMoveBeh() = default;
	virtual bool Move(float deltaTime) const = 0;
};
