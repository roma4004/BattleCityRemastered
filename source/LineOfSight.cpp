#include "../headers/LineOfSight.h"
#include "../headers/BaseObj.h"
#include "../headers/Direction.h"
#include "../headers/Point.h"
#include "../headers/utils/ColliderUtils.h"

#include <algorithm>

LineOfSight::LineOfSight(const ObjRectangle tankShape, const UPoint& windowSize, const FPoint bulletSize,
                         std::vector<std::shared_ptr<BaseObj>>* allObjects, const BaseObj* excludeSelf)
	: _allObjects{allObjects}
{
	const FPoint fWindowSize = {.x = static_cast<float>(windowSize.x), .y = static_cast<float>(windowSize.y)};
	const FPoint tankHalf = {.x = tankShape.w / 2.f, .y = tankShape.h / 2.f};
	const FPoint tankCenter = {.x = tankShape.x + tankHalf.x, .y = tankShape.y + tankHalf.y};
	const float tankDownY = {tankShape.y + tankShape.h};
	const float tankRightX = {tankShape.x + tankShape.w};
	const FPoint bulletSpawnPos = {tankCenter.x - bulletSize.x, tankCenter.y - bulletSize.y};
	const FPoint sightSize = {fWindowSize.x - tankRightX, fWindowSize.y - tankDownY};

	_checkLos = std::vector<ObjRectangle>{
			/*up, left, down, right*///TODO: align to not needed exclude self
			{.x = bulletSpawnPos.x, .y = 0.f, .w = bulletSize.x, .h = tankShape.y},
			{.x = 0.f, .y = bulletSpawnPos.y, .w = tankShape.x, .h = bulletSize.y},
			{.x = bulletSpawnPos.x, .y = tankDownY, .w = bulletSize.x, .h = sightSize.y},
			{.x = tankRightX, .y = bulletSpawnPos.y, .w = sightSize.x, .h = bulletSize.y}
	};

	CheckLOS(excludeSelf);
}

LineOfSight::LineOfSight(const ObjRectangle tankShape, const UPoint& windowSize,
                         std::vector<std::shared_ptr<BaseObj>>* allObjects, const BaseObj* excludeSelf)
	: _allObjects{allObjects}
{
	const FPoint fWindowSize = {.x = static_cast<float>(windowSize.x), .y = static_cast<float>(windowSize.y)};
	const float tankDownY = {tankShape.y + tankShape.h};
	const float tankRightX = {tankShape.x + tankShape.w};
	const FPoint sightSize = {fWindowSize.x - tankRightX, fWindowSize.y - tankDownY};

	_checkLos = std::vector<ObjRectangle>{
			/*up, left, down, right*/
			{.x = tankShape.x, .y = 0.f, .w = tankShape.w, .h = tankShape.y},
			{.x = 0.f, .y = tankShape.y, .w = tankShape.x, .h = tankShape.h},
			{.x = tankShape.x, .y = tankDownY, .w = tankShape.w, .h = sightSize.y},
			{.x = tankRightX, .y = tankShape.y, .w = sightSize.x, .h = tankShape.h}
	};

	CheckLOS(excludeSelf);
}

void LineOfSight::CheckLOS(const BaseObj* excludeSelf)
{
	// parse all seen in Line Of Sight obj
	for (std::shared_ptr<BaseObj>& object: *_allObjects)
	{
		if (excludeSelf == object.get())
		{
			continue;
		}

		if (!object->GetIsPassable() && !object->GetIsPenetrable())
		{
			if (ColliderUtils::IsCollide(_checkLos[UP], object->GetShape()))
			{
				_upSideObstacles.emplace_back(std::weak_ptr(object));
			}

			if (ColliderUtils::IsCollide(_checkLos[LEFT], object->GetShape()))
			{
				_leftSideObstacles.emplace_back(std::weak_ptr(object));
			}

			if (ColliderUtils::IsCollide(_checkLos[DOWN], object->GetShape()))
			{
				_downSideObstacles.emplace_back(std::weak_ptr(object));
			}

			if (ColliderUtils::IsCollide(_checkLos[RIGHT], object->GetShape()))
			{
				_rightSideObstacles.emplace_back(std::weak_ptr(object));
			}
		}
	}

	SortToNearest();
}

void LineOfSight::SortToNearest()
{
	std::ranges::sort(_upSideObstacles, [](const std::weak_ptr<BaseObj>& a, const std::weak_ptr<BaseObj>& b)
	{
		const auto aLck = a.lock();
		if (!aLck)
		{
			return false;
		}

		const auto bLck = b.lock();
		if (!bLck)
		{
			return true;
		}

		return aLck->GetPos().y > bLck->GetPos().y;
	});

	std::ranges::sort(_leftSideObstacles, [](const std::weak_ptr<BaseObj>& a, const std::weak_ptr<BaseObj>& b)
	{
		const auto aLck = a.lock();
		if (!aLck)
		{
			return false;
		}

		const auto bLck = b.lock();
		if (!bLck)
		{
			return true;
		}

		return aLck->GetPos().x > bLck->GetPos().x;
	});

	std::ranges::sort(_downSideObstacles, [](const std::weak_ptr<BaseObj>& a, const std::weak_ptr<BaseObj>& b)
	{
		const auto aLck = a.lock();
		if (!aLck)
		{
			return false;
		}

		const auto bLck = b.lock();
		if (!bLck)
		{
			return true;
		}

		return aLck->GetPos().y < bLck->GetPos().y;
	});

	std::ranges::sort(_rightSideObstacles, [](const std::weak_ptr<BaseObj>& a, const std::weak_ptr<BaseObj>& b)
	{
		const auto aLck = a.lock();
		if (!aLck)
		{
			return false;
		}

		const auto bLck = b.lock();
		if (!bLck)
		{
			return true;
		}

		return aLck->GetPos().x < bLck->GetPos().x;
	});
}

LineOfSight::~LineOfSight() = default;

std::vector<std::weak_ptr<BaseObj>>& LineOfSight::GetUpSideObstacles() { return _upSideObstacles; }
std::vector<std::weak_ptr<BaseObj>>& LineOfSight::GetLeftSideObstacles() { return _leftSideObstacles; }
std::vector<std::weak_ptr<BaseObj>>& LineOfSight::GetDownSideObstacles() { return _downSideObstacles; }
std::vector<std::weak_ptr<BaseObj>>& LineOfSight::GetRightSideObstacles() { return _rightSideObstacles; }
