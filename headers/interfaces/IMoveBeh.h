#pragma once

#include <memory>
#include <vector>

enum class Direction : char8_t;
class BaseObj;

class IMoveBeh
{
	[[nodiscard]] virtual bool IsCanMove(double deltaTime) const = 0;

protected:
	[[nodiscard]] virtual bool MoveLeft(std::vector<std::shared_ptr<BaseObj>>& outCollisions, double deltaTime) = 0;
	[[nodiscard]] virtual bool MoveRight(std::vector<std::shared_ptr<BaseObj>>& outCollisions, double deltaTime) =	0;
	[[nodiscard]] virtual bool MoveUp(std::vector<std::shared_ptr<BaseObj>>& outCollisions, double deltaTime) = 0;
	[[nodiscard]] virtual bool MoveDown(std::vector<std::shared_ptr<BaseObj>>& outCollisions, double deltaTime) = 0;

public:
	virtual ~IMoveBeh() = default;

	[[nodiscard]] virtual bool Move(std::vector<std::shared_ptr<BaseObj>>& outCollisions, double deltaTime) = 0;

	[[nodiscard]] virtual std::vector<Direction> GetFreePathSides(double deltaTime) const = 0;
};
