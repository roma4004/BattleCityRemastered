#pragma once

#include <memory>
#include <optional>
#include <vector>

enum class Direction : char8_t;
class BaseObj;

class IMoveBeh
{
	[[nodiscard]] virtual bool IsCanMove(double deltaTime, Direction dir,
										 const std::vector<std::shared_ptr<BaseObj>>& objects) const = 0;

protected:
	[[nodiscard]] virtual bool MoveUp(double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects,
									  std::vector<std::shared_ptr<BaseObj>>& outCollisions) = 0;
	[[nodiscard]] virtual bool MoveLeft(double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects,
										std::vector<std::shared_ptr<BaseObj>>& outCollisions) = 0;
	[[nodiscard]] virtual bool MoveDown(double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects,
										std::vector<std::shared_ptr<BaseObj>>& outCollisions) = 0;
	[[nodiscard]] virtual bool MoveRight(double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects,
										 std::vector<std::shared_ptr<BaseObj>>& outCollisions) = 0;

public:
	virtual ~IMoveBeh() = default;

	[[nodiscard]] virtual bool Move(Direction direction, double deltaTime,
									const std::vector<std::shared_ptr<BaseObj>>& objects,
									std::vector<std::shared_ptr<BaseObj>>& outCollisions) = 0;
	[[nodiscard]] virtual std::vector<Direction> GetFreePathSides(
			double deltaTime, std::optional<Direction> excludeDirection,
			const std::vector<std::shared_ptr<BaseObj>>& objects) const = 0;
};
