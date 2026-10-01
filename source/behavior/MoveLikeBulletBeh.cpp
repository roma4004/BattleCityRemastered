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
									 const GameConfig& gameConfig, const BulletCaliber& caliber)
	: _uuid{uuid}
	, _authorUuid{authorUuid}
	, _rect{rect}
	, _gameConfig{gameConfig}
	, _caliber{caliber} {}

double MoveLikeBulletBeh::GetTravelledDistance(const double deltaTime, const Direction dir,
											   const std::vector<std::shared_ptr<BaseObj>>& objects) const
{
	const double step{_caliber.speed * deltaTime};
	const ObjRectangle nextPosRect{DirectionUtils::Swept(_rect, step, dir)};

	double travelled{std::min(step, DirectionUtils::GapToEdge(_rect, _gameConfig.battlefieldSize, dir))};

	for (const std::shared_ptr<BaseObj>& object: objects)
	{
		if (!IsInTheWay(object, nextPosRect))
		{
			continue;
		}

		travelled = std::min(travelled, DirectionUtils::GapTo(_rect, object->GetRect(), dir));
	}

	return std::max(0.0, travelled);
}

FPoint MoveLikeBulletBeh::GetBlowCenter(const double deltaTime, const Direction dir,
										const std::vector<std::shared_ptr<BaseObj>>& objects) const
{
	return DirectionUtils::Moved(_rect.Center(), GetTravelledDistance(deltaTime, dir, objects), dir);
}

bool MoveLikeBulletBeh::IsSelfOrAuthor(const BaseObj& object) const
{
	const Uuid objectUuid{object.GetUuid()};

	return objectUuid == _uuid || (_authorUuid != Uuid{} && objectUuid == _authorUuid);
}

bool MoveLikeBulletBeh::IsInTheWay(const std::shared_ptr<BaseObj>& object, const ObjRectangle& nextPosRect) const
{
	return ObjectUtils::IsAlive(object)
		   && !IsSelfOrAuthor(*object)
		   && ColliderUtils::IsCollide(nextPosRect, object->GetRect())
		   && !object->GetIsPenetrable();
}

bool MoveLikeBulletBeh::IsCanMove(const double deltaTime, const Direction dir,
								  const std::vector<std::shared_ptr<BaseObj>>& objects) const
{
	const ObjRectangle nextPosRect{DirectionUtils::Swept(_rect, _caliber.speed * deltaTime, dir)};

	return std::ranges::none_of(objects, [this, nextPosRect](const std::shared_ptr<BaseObj>& object)
	{
		return IsInTheWay(object, nextPosRect);
	});
}

bool MoveLikeBulletBeh::Move(const Direction dir, const double deltaTime,
							 const std::vector<std::shared_ptr<BaseObj>>& objects,
							 std::vector<std::shared_ptr<BaseObj>>& outCollisions)
{
	const double speed{_caliber.speed * deltaTime};
	if (speed <= DirectionUtils::GapToEdge(_rect, _gameConfig.battlefieldSize, dir)
		&& IsCanMove(deltaTime, dir, objects))
	{
		_rect = DirectionUtils::Moved(_rect, speed, dir);

		return true;
	}

	//NOTE: stopped by the edge or by an obstacle - the blast hits everything around where it stopped
	outCollisions = GetCircleCollisionObjects(GetBlowCenter(deltaTime, dir, objects), objects);

	return false;
}

std::vector<std::shared_ptr<BaseObj>> MoveLikeBulletBeh::GetContacts(
		const Direction dir, const double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects) const
{
	const ObjRectangle nextPosRect{DirectionUtils::Swept(_rect, _caliber.speed * deltaTime, dir)};
	const double travelled{GetTravelledDistance(deltaTime, dir, objects)};

	auto contacts{objects | std::views::filter([this, &nextPosRect, travelled, dir](const std::shared_ptr<BaseObj>& obj)
	{
		return IsInTheWay(obj, nextPosRect) && DirectionUtils::GapTo(_rect, obj->GetRect(), dir) <= travelled;
	})};

	return std::vector<std::shared_ptr<BaseObj>>{contacts.begin(), contacts.end()};
}

std::vector<std::shared_ptr<BaseObj>> MoveLikeBulletBeh::GetCircleCollisionObjects(
		const FPoint blowCenter, const std::vector<std::shared_ptr<BaseObj>>& objects) const
{
	const Circle circle{.center = blowCenter, .radius = _caliber.damageRadius};

	auto collisions{objects | std::views::filter([this, &circle](const std::shared_ptr<BaseObj>& obj)
	{
		return ObjectUtils::IsAlive(obj)
			   && obj->GetUuid() != _uuid
			   && ColliderUtils::IsCollide(circle, obj->GetRect());
	})};

	return std::vector<std::shared_ptr<BaseObj>>{collisions.begin(), collisions.end()};
}

