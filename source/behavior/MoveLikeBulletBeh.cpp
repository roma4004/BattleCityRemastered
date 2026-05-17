#include "behavior/MoveLikeBulletBeh.h"
#include "Circle.h"
#include "components/EventSystem.h"
#include "entities/pawns/Bullet.h"
#include "enums/Direction.h"
#include "utils/ColliderUtils.h"
#include <algorithm>
#include <memory>

MoveLikeBulletBeh::MoveLikeBulletBeh(ObjRectangle& rect, Direction& dir, float& speed, buuid& uuid,
									 double& damageRadius, UPoint& windowSize,
									 std::vector<std::shared_ptr<BaseObj>>* allObjects)
	: _uuid{uuid}
	, _rect{rect}
	, _direction{dir}
	, _speed{speed}
	, _bulletDamageRadius{damageRadius}
	, _windowSize{windowSize}
	, _allObjects{allObjects} {}

//NOTE: Never user for bullets
std::vector<Direction> MoveLikeBulletBeh::GetFreePathSides(double /*deltaTime*/) const { return {}; }

ObjRectangle MoveLikeBulletBeh::GetBulletPathRect(const double deltaTime) const
{
	const float speed = _speed * static_cast<float>(deltaTime);
	const auto [x, y, w, h] = _rect;
	if (_direction == Direction::UP)
	{
		return {.x = x, .y = y - speed, .w = w, .h = h + speed};
	}

	if (_direction == Direction::DOWN)
	{
		return {.x = x, .y = y, .w = w, .h = h + speed};
	}

	if (_direction == Direction::LEFT)
	{
		//TODO: write bullet test that can damage tank from all sides
		return {.x = x - speed, .y = y, .w = w + speed, .h = h};
	}

	//_direction == RIGHT
	return {.x = x, .y = y, .w = w + speed, .h = h};
}

FPoint MoveLikeBulletBeh::GetBulletNextPoint(const double deltaTime) const
{
	const float speed = _speed * static_cast<float>(deltaTime);
	const auto [x, y, w, h] = _rect;
	if (_direction == Direction::UP)
	{
		return {.x = x, .y = y - speed};
	}

	if (_direction == Direction::DOWN)
	{
		return {.x = x, .y = y + speed};
	}

	if (_direction == Direction::LEFT)
	{
		return {.x = x - speed, .y = y};//TODO: write bullet test that can damage tank from all sides
	}

	//_direction == Direction::RIGHT
	return {.x = x + speed, .y = y};
}

bool MoveLikeBulletBeh::IsCanMove(const double deltaTime) const
{
	return std::ranges::all_of(*_allObjects, [this, deltaTime](const std::shared_ptr<BaseObj>& object)
	{
		if (_uuid == object->GetUuid())
		{
			return true;
		}

		if (ColliderUtils::IsCollide(GetBulletPathRect(deltaTime), object->GetRect()))
		{
			if (!object->GetIsPenetrable())
			{
				return false;
				//TODO: fix move to a bonus though water
			}
		}

		return true;
	});
}

bool MoveLikeBulletBeh::Move(std::vector<std::shared_ptr<BaseObj>>& outCollisions, const double deltaTime)
{
	const float speed = _speed * static_cast<float>(deltaTime);
	const Direction direction = _direction;
	if (direction == Direction::UP && _rect.y - speed >= 0.0f)
	{
		return MoveUp(outCollisions, deltaTime);
	}

	if (direction == Direction::DOWN && _rect.Bottom() + speed <= static_cast<float>(_windowSize.y))
	{
		return MoveDown(outCollisions, deltaTime);
	}

	if (direction == Direction::LEFT && _rect.x - speed >= 0.0f)
	{
		return MoveLeft(outCollisions, deltaTime);
	}

	if (constexpr int sideBarWidth = 175;//TODO: move sidebar width to params
		direction == Direction::RIGHT && _rect.Right() + speed <= static_cast<float>(_windowSize.x) - sideBarWidth)
	{
		return MoveRight(outCollisions, deltaTime);
	}

	// Self-destroy with deal damage when the edge of windows is reached
	outCollisions = GetCircleCollisionObjects(GetBulletNextPoint(deltaTime));

	return false;
}

bool MoveLikeBulletBeh::MoveLeft(std::vector<std::shared_ptr<BaseObj>>& outCollisions, const double deltaTime)
{
	if (IsCanMove(deltaTime))
	{
		_rect.x += -_speed * static_cast<float>(deltaTime);

		return true;
	}

	outCollisions = GetCircleCollisionObjects(GetBulletNextPoint(deltaTime));

	return false;
}

bool MoveLikeBulletBeh::MoveRight(std::vector<std::shared_ptr<BaseObj>>& outCollisions, const double deltaTime)
{
	if (IsCanMove(deltaTime))
	{
		_rect.x += _speed * static_cast<float>(deltaTime);

		return true;
	}

	outCollisions = GetCircleCollisionObjects(GetBulletNextPoint(deltaTime));

	return false;
}

bool MoveLikeBulletBeh::MoveUp(std::vector<std::shared_ptr<BaseObj>>& outCollisions, const double deltaTime)
{
	if (IsCanMove(deltaTime))
	{
		_rect.y += -_speed * static_cast<float>(deltaTime);

		return true;
	}

	outCollisions = GetCircleCollisionObjects(GetBulletNextPoint(deltaTime));

	return false;
}

bool MoveLikeBulletBeh::MoveDown(std::vector<std::shared_ptr<BaseObj>>& outCollisions, const double deltaTime)
{
	if (IsCanMove(deltaTime))
	{
		_rect.y += _speed * static_cast<float>(deltaTime);

		return true;
	}

	outCollisions = GetCircleCollisionObjects(GetBulletNextPoint(deltaTime));

	return false;
}


std::vector<std::shared_ptr<BaseObj>> MoveLikeBulletBeh::GetCircleCollisionObjects(const FPoint blowCenter) const
{
	std::vector<std::shared_ptr<BaseObj>> aoeCollisions{};
	constexpr int defaultCollisionReserve{5};
	aoeCollisions.reserve(defaultCollisionReserve);

	const Circle circle{.center = blowCenter, .radius = _bulletDamageRadius};
	for (const std::shared_ptr<BaseObj>& object: *_allObjects)
	{
		if (_uuid == object->GetUuid())
		{
			continue;
		}

		if (ColliderUtils::IsCollide(circle, object->GetRect()))
		{
			aoeCollisions.emplace_back(object);
		}
	}

	return aoeCollisions;
}
