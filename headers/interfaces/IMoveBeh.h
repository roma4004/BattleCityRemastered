#pragma once

#include <memory>
#include <vector>

enum class Direction : char8_t;
class BaseObj;

class IMoveBeh
{
	[[nodiscard]] virtual bool IsCanMove(double deltaTime) const = 0;

protected:
	[[nodiscard]] virtual bool MoveLeft(double deltaTime) = 0;
	[[nodiscard]] virtual bool MoveRight(double deltaTime) = 0;
	[[nodiscard]] virtual bool MoveUp(double deltaTime) = 0;
	[[nodiscard]] virtual bool MoveDown(double deltaTime) = 0;

public:
	virtual ~IMoveBeh() = default;

	[[nodiscard]] virtual bool Move(double deltaTime) = 0;

	[[nodiscard]] virtual std::vector<Direction> GetFreePathSides(double deltaTime) const = 0;
};
