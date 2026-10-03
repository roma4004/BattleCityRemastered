#pragma once

#include <memory>

class BaseObj;
class Bullet;

class ObjectUtils final
{
public:
	//NOTE: a dead object stays in _allObjects until the PostTickUpdate sweep, so every pass must skip it
	[[nodiscard]] static bool IsAlive(const BaseObj* object);

	[[nodiscard]] static bool IsOpponent(const BaseObj& self, const BaseObj& other);
	[[nodiscard]] static bool IsAlly(const BaseObj& self, const BaseObj& other);
	[[nodiscard]] static bool IsBonus(const BaseObj& object);
	[[nodiscard]] static const Bullet* AsBullet(const BaseObj& object);
	[[nodiscard]] static bool IsFortress(const BaseObj& object);
	[[nodiscard]] static bool IsWall(const BaseObj& object);

	//NOTE: the world holds its objects for the whole tick - what a pass collects is looked at, not kept
	[[nodiscard]] static BaseObj* Raw(const std::shared_ptr<BaseObj>& object) { return object.get(); }
};
