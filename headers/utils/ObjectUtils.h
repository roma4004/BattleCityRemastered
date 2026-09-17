#pragma once

#include <memory>

class BaseObj;

class ObjectUtils final
{
public:
	//NOTE: a dead object stays in _allObjects until the PostTickUpdate sweep, so every pass must skip it
	[[nodiscard]] static bool IsAlive(const std::shared_ptr<BaseObj>& object);
};
