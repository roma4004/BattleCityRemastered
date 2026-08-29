#include "behavior/MoveLikeBulletBeh.h"
#include "geometry/Circle.h"
#include "geometry/ObjRectangle.h"
#include "application/GameConfig.h"
#include "entities/pawns/Bullet.h"
#include "enums/Direction.h"
#include "utils/ColliderUtils.h"
#include <algorithm>
#include <memory>
#include <ranges>

MoveLikeBulletBeh::MoveLikeBulletBeh(ObjRectangle& rect, Direction& dir, Uuid& uuid, const Uuid& authorUuid,
									 const GameConfig& gameConfig, const BulletCalibre& calibre,
									 std::vector<std::shared_ptr<BaseObj>>* allObjects)
	: _uuid{uuid}
	, _authorUuid{authorUuid}
	, _rect{rect}
	, _direction{dir}
	, _gameConfig{gameConfig}
	, _calibre{calibre}
	, _allObjects{allObjects} {}

ObjRectangle MoveLikeBulletBeh::GetNextPos(const double deltaTime) const
{
	const float speed = _calibre.speed * static_cast<float>(deltaTime);
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

float MoveLikeBulletBeh::GetGapTo(const ObjRectangle& target) const
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

float MoveLikeBulletBeh::GetGapToBattlefieldEdge() const
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
		return static_cast<float>(_gameConfig.battlefieldSize.y) - _rect.Bottom();
	}

	//_direction == Direction::RIGHT
	return static_cast<float>(_gameConfig.battlefieldSize.x) - _rect.Right();
}

float MoveLikeBulletBeh::GetTravelledDistance(const double deltaTime) const
{
	const float step = _calibre.speed * static_cast<float>(deltaTime);
	const ObjRectangle nextPosRect = GetNextPos(deltaTime);

	float travelled = std::min(step, GetGapToBattlefieldEdge());

	for (const std::shared_ptr<BaseObj>& object: *_allObjects)
	{
		if (IsSelfOrAuthor(*object) || object->GetIsPenetrable()
			|| !ColliderUtils::IsCollide(nextPosRect, object->GetRect()))
		{
			continue;
		}

		travelled = std::min(travelled, GetGapTo(object->GetRect()));
	}

	return std::max(0.f, travelled);
}

// Where the bullet stopped, not where the frame step would have taken it - the blast is centred there
FPoint MoveLikeBulletBeh::GetBlowCenter(const double deltaTime) const
{
	const float travelled = GetTravelledDistance(deltaTime);
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

bool MoveLikeBulletBeh::IsCanMove(const double deltaTime, const Direction /*dir*/) const
{
	const ObjRectangle nextPosRect = GetNextPos(deltaTime);

	return std::ranges::none_of(*_allObjects, [this, nextPosRect](const std::shared_ptr<BaseObj>& object)
	{
		return !IsSelfOrAuthor(*object)
			   && ColliderUtils::IsCollide(nextPosRect, object->GetRect())
			   && !object->GetIsPenetrable();
	});
}

bool MoveLikeBulletBeh::Move(const Direction dir, const double deltaTime,
							 std::vector<std::shared_ptr<BaseObj>>& outCollisions)
{
	const float speed = _calibre.speed * static_cast<float>(deltaTime);
	if (dir == Direction::UP && _rect.y - speed >= 0.0f)
	{
		return MoveUp(deltaTime, outCollisions);
	}

	if (dir == Direction::LEFT && _rect.x - speed >= 0.0f)
	{
		return MoveLeft(deltaTime, outCollisions);
	}

	if (dir == Direction::DOWN && _rect.Bottom() + speed <= static_cast<float>(_gameConfig.battlefieldSize.y))
	{
		return MoveDown(deltaTime, outCollisions);
	}

	if (dir == Direction::RIGHT
		&& _rect.Right() + speed <= static_cast<float>(_gameConfig.battlefieldSize.x))
	{
		return MoveRight(deltaTime, outCollisions);
	}

	// Self-destroy with deal damage when the edge of windows is reached
	outCollisions = GetCircleCollisionObjects(GetBlowCenter(deltaTime));

	return false;
}

bool MoveLikeBulletBeh::MoveUp(const double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions)
{
	if (IsCanMove(deltaTime, _direction))
	{
		_rect.y += -_calibre.speed * static_cast<float>(deltaTime);

		return true;
	}

	outCollisions = GetCircleCollisionObjects(GetBlowCenter(deltaTime));

	return false;
}

bool MoveLikeBulletBeh::MoveLeft(const double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions)
{
	if (IsCanMove(deltaTime, _direction))
	{
		_rect.x += -_calibre.speed * static_cast<float>(deltaTime);

		return true;
	}

	outCollisions = GetCircleCollisionObjects(GetBlowCenter(deltaTime));

	return false;
}

bool MoveLikeBulletBeh::MoveDown(const double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions)
{
	if (IsCanMove(deltaTime, _direction))
	{
		_rect.y += _calibre.speed * static_cast<float>(deltaTime);

		return true;
	}

	outCollisions = GetCircleCollisionObjects(GetBlowCenter(deltaTime));

	return false;
}

bool MoveLikeBulletBeh::MoveRight(const double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions)
{
	if (IsCanMove(deltaTime, _direction))
	{
		_rect.x += _calibre.speed * static_cast<float>(deltaTime);

		return true;
	}

	outCollisions = GetCircleCollisionObjects(GetBlowCenter(deltaTime));

	return false;
}

std::vector<std::shared_ptr<BaseObj>> MoveLikeBulletBeh::GetCircleCollisionObjects(const FPoint blowCenter) const
{
	const Circle circle{.center = blowCenter, .radius = _calibre.damageRadius};

	auto collisions = *_allObjects | std::views::filter([this, &circle](const std::shared_ptr<BaseObj>& obj)
	{
		return obj->GetUuid() != _uuid
			   && ColliderUtils::IsCollide(circle, obj->GetRect());
	});

	return std::vector<std::shared_ptr<BaseObj>>{collisions.begin(), collisions.end()};
}

void MoveLikeBulletBeh::Reset(const BulletCalibre& calibre) { _calibre = calibre; }

std::vector<Direction> MoveLikeBulletBeh::GetFreePathSides(const double /*deltaTime*/,
														   const std::optional<Direction> /*excludeDirection*/) const
{
	return {};
}
