#pragma once

#include <memory>

class BaseObj;
class Bullet;

class ObjectUtils final
{
public:
	//NOTE: a dead object stays in _allObjects until the PostTickUpdate sweep, so every pass must skip it
	[[nodiscard]] static bool IsAlive(const std::shared_ptr<BaseObj>& object);

	[[nodiscard]] static bool IsOpponent(const BaseObj& self, const std::shared_ptr<BaseObj>& other);
	[[nodiscard]] static bool IsAlly(const BaseObj& self, const std::shared_ptr<BaseObj>& other);
	[[nodiscard]] static bool IsBonus(const std::shared_ptr<BaseObj>& object);
	[[nodiscard]] static const Bullet* AsBullet(const std::shared_ptr<BaseObj>& object);
	[[nodiscard]] static bool IsFortress(const std::shared_ptr<BaseObj>& object);
	[[nodiscard]] static bool IsWall(const std::shared_ptr<BaseObj>& object);
};
