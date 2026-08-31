#include "behavior/MoveLikeBulletBeh.h"
#include "geometry/Circle.h"
#include "geometry/ObjRectangle.h"
#include "application/GameConfig.h"
#include "entities/pawns/Bullet.h"
#include "enums/Direction.h"
#include "utils/ColliderUtils.h"
#include "utils/ObjectUtils.h"
#include <algorithm>
#include <memory>
#include <ranges>

MoveLikeBulletBeh::MoveLikeBulletBeh(ObjRectangle& rect, Direction& dir, Uuid& uuid, const Uuid& authorUuid,
									 const GameConfig& gameConfig, const BulletCalibre& calibre)
	: _uuid{uuid}
	, _authorUuid{authorUuid}
	, _rect{rect}
	, _direction{dir}
	, _gameConfig{gameConfig}
	, _calibre{calibre} {}

ObjRectangle MoveLikeBulletBeh::GetNextPos(const double deltaTime) const
{
	const double speed = _calibre.speed * deltaTime;
	const auto [x, y, w, h] = _rect;
	if (_direction == Direction::UP)
	{
		return ObjRectangle{.x = x, .y = y - speed, .w = w, .h = h + speed};
	}

	if (_direction == Direction::LEFT)
	{
		return ObjRectangle{.x = x - speed, .y = y, .w = w + speed, .h = h};
	}

	if (_direction == Direction::DOWN)
	{
		return ObjRectangle{.x = x, .y = y, .w = w, .h = h + speed};
	}

	//_direction == RIGHT
	// {
	return ObjRectangle{.x = x, .y = y, .w = w + speed, .h = h};
	// }
}

double MoveLikeBulletBeh::GetGapTo(const ObjRectangle& target) const
{
	if (_direction == Direction::UP)
	{
		return _rect.y - target.Bottom();
	}

	if (_direction == Direction::LEFT)
	{
		return _rect.x - target.Right();
	}

	if (_direction == Direction::DOWN)
	{
		return target.y - _rect.Bottom();
	}

	//_direction == Direction::RIGHT
	return target.x - _rect.Right();
}

double MoveLikeBulletBeh::GetGapToBattlefieldEdge() const
{
	if (_direction == Direction::UP)
	{
		return _rect.y;
	}

	if (_direction == Direction::LEFT)
	{
		return _rect.x;
	}

	if (_direction == Direction::DOWN)
	{
		return static_cast<double>(_gameConfig.battlefieldSize.y) - _rect.Bottom();
	}

	//_direction == Direction::RIGHT
	return static_cast<double>(_gameConfig.battlefieldSize.x) - _rect.Right();
}

double MoveLikeBulletBeh::GetTravelledDistance(const double deltaTime,
											   const std::vector<std::shared_ptr<BaseObj>>& objects) const
{
	const double step = _calibre.speed * deltaTime;
	const ObjRectangle nextPosRect = GetNextPos(deltaTime);

	double travelled = std::min(step, GetGapToBattlefieldEdge());

	for (const std::shared_ptr<BaseObj>& object: objects)
	{
		if (!ObjectUtils::IsAlive(object) || IsSelfOrAuthor(*object) || object->GetIsPenetrable()
			|| !ColliderUtils::IsCollide(nextPosRect, object->GetRect()))
		{
			continue;
		}

		travelled = std::min(travelled, GetGapTo(object->GetRect()));
	}

	return std::max(0.0, travelled);
}

// Where the bullet stopped, not where the frame step would have taken it - the blast is centred there
FPoint MoveLikeBulletBeh::GetBlowCenter(const double deltaTime,
										const std::vector<std::shared_ptr<BaseObj>>& objects) const
{
	const double travelled = GetTravelledDistance(deltaTime, objects);
	const auto [x, y] = _rect.Center();
	if (_direction == Direction::UP)
	{
		return FPoint{.x = x, .y = y - travelled};
	}

	if (_direction == Direction::LEFT)
	{
		return FPoint{.x = x - travelled, .y = y};
	}

	if (_direction == Direction::DOWN)
	{
		return FPoint{.x = x, .y = y + travelled};
	}

	//_direction == Direction::RIGHT
	return FPoint{.x = x + travelled, .y = y};
}

bool MoveLikeBulletBeh::IsSelfOrAuthor(const BaseObj& object) const
{
	const Uuid objectUuid = object.GetUuid();

	return objectUuid == _uuid || (_authorUuid != Uuid{} && objectUuid == _authorUuid);
}

bool MoveLikeBulletBeh::IsCanMove(const double deltaTime, const Direction /*dir*/,
								  const std::vector<std::shared_ptr<BaseObj>>& objects) const
{
	const ObjRectangle nextPosRect = GetNextPos(deltaTime);

	return std::ranges::none_of(objects, [this, nextPosRect](const std::shared_ptr<BaseObj>& object)
	{
		return ObjectUtils::IsAlive(object)
			   && !IsSelfOrAuthor(*object)
			   && ColliderUtils::IsCollide(nextPosRect, object->GetRect())
			   && !object->GetIsPenetrable();
	});
}

bool MoveLikeBulletBeh::Move(const Direction dir, const double deltaTime,
							 const std::vector<std::shared_ptr<BaseObj>>& objects,
							 std::vector<std::shared_ptr<BaseObj>>& outCollisions)
{
	const double speed = _calibre.speed * deltaTime;
	if (dir == Direction::UP && _rect.y - speed >= 0.0)
	{
		return MoveUp(deltaTime, objects, outCollisions);
	}

	if (dir == Direction::LEFT && _rect.x - speed >= 0.0)
	{
		return MoveLeft(deltaTime, objects, outCollisions);
	}

	if (dir == Direction::DOWN && _rect.Bottom() + speed <= static_cast<double>(_gameConfig.battlefieldSize.y))
	{
		return MoveDown(deltaTime, objects, outCollisions);
	}

	if (dir == Direction::RIGHT
		&& _rect.Right() + speed <= static_cast<double>(_gameConfig.battlefieldSize.x))
	{
		return MoveRight(deltaTime, objects, outCollisions);
	}

	// Self-destroy with deal damage when the edge of windows is reached
	outCollisions = GetCircleCollisionObjects(GetBlowCenter(deltaTime, objects), objects);

	return false;
}

bool MoveLikeBulletBeh::MoveUp(const double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects,
							   std::vector<std::shared_ptr<BaseObj>>& outCollisions)
{
	if (IsCanMove(deltaTime, _direction, objects))
	{
		_rect.y += -_calibre.speed * deltaTime;

		return true;
	}

	outCollisions = GetCircleCollisionObjects(GetBlowCenter(deltaTime, objects), objects);

	return false;
}

bool MoveLikeBulletBeh::MoveLeft(const double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects,
								 std::vector<std::shared_ptr<BaseObj>>& outCollisions)
{
	if (IsCanMove(deltaTime, _direction, objects))
	{
		_rect.x += -_calibre.speed * deltaTime;

		return true;
	}

	outCollisions = GetCircleCollisionObjects(GetBlowCenter(deltaTime, objects), objects);

	return false;
}

bool MoveLikeBulletBeh::MoveDown(const double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects,
								 std::vector<std::shared_ptr<BaseObj>>& outCollisions)
{
	if (IsCanMove(deltaTime, _direction, objects))
	{
		_rect.y += _calibre.speed * deltaTime;

		return true;
	}

	outCollisions = GetCircleCollisionObjects(GetBlowCenter(deltaTime, objects), objects);

	return false;
}

bool MoveLikeBulletBeh::MoveRight(const double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects,
								  std::vector<std::shared_ptr<BaseObj>>& outCollisions)
{
	if (IsCanMove(deltaTime, _direction, objects))
	{
		_rect.x += _calibre.speed * deltaTime;

		return true;
	}

	outCollisions = GetCircleCollisionObjects(GetBlowCenter(deltaTime, objects), objects);

	return false;
}

std::vector<std::shared_ptr<BaseObj>> MoveLikeBulletBeh::GetCircleCollisionObjects(
		const FPoint blowCenter, const std::vector<std::shared_ptr<BaseObj>>& objects) const
{
	const Circle circle{.center = blowCenter, .radius = _calibre.damageRadius};

	auto collisions = objects | std::views::filter([this, &circle](const std::shared_ptr<BaseObj>& obj)
	{
		return ObjectUtils::IsAlive(obj)
			   && obj->GetUuid() != _uuid
			   && ColliderUtils::IsCollide(circle, obj->GetRect());
	});

	return std::vector<std::shared_ptr<BaseObj>>{collisions.begin(), collisions.end()};
}

void MoveLikeBulletBeh::Reset(const BulletCalibre& calibre) { _calibre = calibre; }

std::vector<Direction> MoveLikeBulletBeh::GetFreePathSides(
		const double /*deltaTime*/, const std::optional<Direction> /*excludeDirection*/,
		const std::vector<std::shared_ptr<BaseObj>>& /*objects*/) const
{
	return {};
}
