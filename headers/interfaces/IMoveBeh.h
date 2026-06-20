#pragma once

#include <memory>
#include <vector>

enum class Direction : char8_t;
class BaseObj;

class IMoveBeh
{
	[[nodiscard]] virtual bool IsCanMove(double deltaTime) const = 0;

protected:
	[[nodiscard]] virtual bool MoveLeft(double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions) = 0;
	[[nodiscard]] virtual bool MoveRight(double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions) = 0;
	[[nodiscard]] virtual bool MoveUp(double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions) = 0;
	[[nodiscard]] virtual bool MoveDown(double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions) = 0;

public:
	virtual ~IMoveBeh() = default;

	[[nodiscard]] virtual bool Move(Direction direction, double deltaTime,
									std::vector<std::shared_ptr<BaseObj>>& outCollisions) = 0;
};
