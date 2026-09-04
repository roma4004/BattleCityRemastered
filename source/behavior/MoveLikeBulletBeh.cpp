#include "behavior/MoveLikeBulletBeh.h"
#include "geometry/Circle.h"
#include "geometry/ObjRectangle.h"
#include "application/GameConfig.h"
#include "entities/pawns/Bullet.h"
#include "enums/Direction.h"
#include "utils/ColliderUtils.h"
#include "utils/DirectionUtils.h"
#include "utils/ObjectUtils.h"
#include <algorithm>
#include <memory>
#include <ranges>

MoveLikeBulletBeh::MoveLikeBulletBeh(ObjRectangle& rect, Uuid& uuid, const Uuid& authorUuid,
									 const GameConfig& gameConfig, const BulletCalibre& calibre)
	: _uuid{uuid}
	, _authorUuid{authorUuid}
	, _rect{rect}
	, _gameConfig{gameConfig}
	, _calibre{calibre} {}

double MoveLikeBulletBeh::GetTravelledDistance(const double deltaTime, const Direction dir,
											   const std::vector<std::shared_ptr<BaseObj>>& objects) const
{
	const double step = _calibre.speed * deltaTime;
	const ObjRectangle nextPosRect = DirectionUtils::Swept(_rect, step, dir);

	double travelled = std::min(step, DirectionUtils::GapToEdge(_rect, _gameConfig.battlefieldSize, dir));

	for (const std::shared_ptr<BaseObj>& object: objects)
	{
		if (!ObjectUtils::IsAlive(object) || IsSelfOrAuthor(*object) || object->GetIsPenetrable()
			|| !ColliderUtils::IsCollide(nextPosRect, object->GetRect()))
		{
			continue;
		}

		travelled = std::min(travelled, DirectionUtils::GapTo(_rect, object->GetRect(), dir));
	}

	return std::max(0.0, travelled);
}

// Where the bullet stopped, not where the frame step would have taken it - the blast is centred there
FPoint MoveLikeBulletBeh::GetBlowCenter(const double deltaTime, const Direction dir,
										const std::vector<std::shared_ptr<BaseObj>>& objects) const
{
	return DirectionUtils::Moved(_rect.Center(), GetTravelledDistance(deltaTime, dir, objects), dir);
}

bool MoveLikeBulletBeh::IsSelfOrAuthor(const BaseObj& object) const
{
	const Uuid objectUuid = object.GetUuid();

	return objectUuid == _uuid || (_authorUuid != Uuid{} && objectUuid == _authorUuid);
}

bool MoveLikeBulletBeh::IsCanMove(const double deltaTime, const Direction dir,
								  const std::vector<std::shared_ptr<BaseObj>>& objects) const
{
	const ObjRectangle nextPosRect = DirectionUtils::Swept(_rect, _calibre.speed * deltaTime, dir);

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
	if (speed <= DirectionUtils::GapToEdge(_rect, _gameConfig.battlefieldSize, dir)
		&& IsCanMove(deltaTime, dir, objects))
	{
		_rect = DirectionUtils::Moved(_rect, speed, dir);

		return true;
	}

	// Self-destroy with deal damage when the edge of windows is reached
	outCollisions = GetCircleCollisionObjects(GetBlowCenter(deltaTime, dir, objects), objects);

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

