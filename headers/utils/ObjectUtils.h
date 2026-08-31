#pragma once

#include <memory>

class BaseObj;

class ObjectUtils final
{
public:
	//NOTE: a corpse lingers in _allObjects until DisposeDeadObject runs on PostTickUpdate, so every
	//pass over the container in between has to skip it instead of treating it as a live object
	[[nodiscard]] static bool IsAlive(const std::shared_ptr<BaseObj>& object);
};
