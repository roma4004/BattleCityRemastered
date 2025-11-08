#pragma once

#include <memory>
#include <vector>

enum class Direction : char8_t;
class BaseObj;

class IMoveBeh
{
	[[nodiscard]] virtual bool IsCanMove(float deltaTime) const = 0;

protected:
	[[nodiscard]] virtual bool MoveLeft(float deltaTime) = 0;
	[[nodiscard]] virtual bool MoveRight(float deltaTime) = 0;
	[[nodiscard]] virtual bool MoveUp(float deltaTime) = 0;
	[[nodiscard]] virtual bool MoveDown(float deltaTime) = 0;

public:
	virtual ~IMoveBeh() = default;

	[[nodiscard]] virtual bool Move(float deltaTime) = 0;

	[[nodiscard]] virtual std::vector<Direction> GetFreePathSides(float deltaTime) const = 0;
};
