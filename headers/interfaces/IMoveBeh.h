#pragma once

#include <memory>
#include <vector>

enum class Direction : char8_t;
class BaseObj;

class IMoveBeh
{
public:
	virtual ~IMoveBeh() = default;

	[[nodiscard]] virtual bool Move(Direction direction, double deltaTime,
									const std::vector<std::shared_ptr<BaseObj>>& objects,
									std::vector<std::shared_ptr<BaseObj>>& outCollisions) = 0;
};
