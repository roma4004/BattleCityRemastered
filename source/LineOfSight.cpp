#include "../headers/LineOfSight.h"
#include "../headers/BaseObj.h"
#include "../headers/Direction.h"
#include "../headers/ObjRectangle.h"
#include "../headers/Point.h"
#include "../headers/utils/ColliderUtils.h"

#include <algorithm>

LineOfSight::LineOfSight(const ObjRectangle shape, const UPoint& windowSize, const FPoint bulletHalf,
                         std::vector<std::shared_ptr<BaseObj>>* allObjects, const BaseObj* excludeSelf)
	: _allObjects{allObjects}
{
	const FPoint fWindowSize = {.x = static_cast<float>(windowSize.x), .y = static_cast<float>(windowSize.y)};
	const FPoint tankHalf = {.x = shape.w / 2.f, .y = shape.h / 2.f};
	checkLOS = std::vector<ObjRectangle>{
			/*up, left, down, right*/
			{.x = shape.x + tankHalf.x - bulletHalf.x, .y = 0.f, .w = bulletHalf.x, .h = shape.y},
			{.x = 0.f, .y = shape.y + tankHalf.y - bulletHalf.y, .w = shape.x, .h = bulletHalf.y},
			{.x = shape.x + tankHalf.x - bulletHalf.x, .y = shape.y + shape.h, .w = bulletHalf.x, .h = fWindowSize.y},
			{.x = shape.x + shape.w, .y = shape.y + tankHalf.y - bulletHalf.y, .w = fWindowSize.x, .h = bulletHalf.y}
	};

	// parse all seen in LOS (line of sight) obj
	for (std::shared_ptr<BaseObj>& object: *_allObjects)
	{
		if (excludeSelf == object.get())
		{
			continue;
		}

		if (!object->GetIsPassable() && !object->GetIsPenetrable())
		{
			if (ColliderUtils::IsCollide(checkLOS[UP], object->GetShape()))
			{
				upSideObstacles.emplace_back(std::weak_ptr(object));
			}

			if (ColliderUtils::IsCollide(checkLOS[LEFT], object->GetShape()))
			{
				leftSideObstacles.emplace_back(std::weak_ptr(object));
			}

			if (ColliderUtils::IsCollide(checkLOS[DOWN], object->GetShape()))
			{
				downSideObstacles.emplace_back(std::weak_ptr(object));
			}

			if (ColliderUtils::IsCollide(checkLOS[RIGHT], object->GetShape()))
			{
				rightSideObstacles.emplace_back(std::weak_ptr(object));
			}
		}
	}

	// sorting to nearest
	std::ranges::sort(upSideObstacles, [](const std::weak_ptr<BaseObj>& a, const std::weak_ptr<BaseObj>& b)
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

	std::ranges::sort(leftSideObstacles, [](const std::weak_ptr<BaseObj>& a, const std::weak_ptr<BaseObj>& b)
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

	std::ranges::sort(downSideObstacles, [](const std::weak_ptr<BaseObj>& a, const std::weak_ptr<BaseObj>& b)
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

	std::ranges::sort(rightSideObstacles, [](const std::weak_ptr<BaseObj>& a, const std::weak_ptr<BaseObj>& b)
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

std::vector<std::weak_ptr<BaseObj>>& LineOfSight::GetUpSideObstacles() { return upSideObstacles; }
std::vector<std::weak_ptr<BaseObj>>& LineOfSight::GetLeftSideObstacles() { return leftSideObstacles; }
std::vector<std::weak_ptr<BaseObj>>& LineOfSight::GetDownSideObstacles() { return downSideObstacles; }
std::vector<std::weak_ptr<BaseObj>>& LineOfSight::GetRightSideObstacles() { return rightSideObstacles; }
