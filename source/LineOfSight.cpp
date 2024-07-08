#include "../headers/LineOfSight.h"
#include "../headers/Direction.h"

#include <algorithm>

LineOfSight::LineOfSight(const Rectangle shape, const UPoint windowSize, const FPoint bulletHalf,
                         std::vector<std::shared_ptr<BaseObj>>* allObjects, const BaseObj* excludeSelf)
	: _allObjects{allObjects}
{
	const FPoint fWindowSize = {static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)};
	const FPoint tankHalf = {shape.w / 2.f, shape.h / 2.f};
	checkLOS = std::vector<Rectangle>{
			/*up, left, down, right*/
			{shape.x + tankHalf.x - bulletHalf.x, 0.f, bulletHalf.x, shape.y},
			{0.f, shape.y + tankHalf.y - bulletHalf.y, shape.x, bulletHalf.y},
			{shape.x + tankHalf.x - bulletHalf.x, shape.y + shape.h, bulletHalf.x, fWindowSize.y},
			{shape.x + shape.w, shape.y + tankHalf.y - bulletHalf.y, fWindowSize.x, bulletHalf.y}
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
			if (ColliderCheck::IsCollide(checkLOS[UP], object->GetShape()))
			{
				upSideObstacles.emplace_back(std::weak_ptr(object));
			}
			if (ColliderCheck::IsCollide(checkLOS[LEFT], object->GetShape()))
			{
				leftSideObstacles.emplace_back(std::weak_ptr(object));
			}
			if (ColliderCheck::IsCollide(checkLOS[DOWN], object->GetShape()))
			{
				downSideObstacles.emplace_back(std::weak_ptr(object));
			}
			if (ColliderCheck::IsCollide(checkLOS[RIGHT], object->GetShape()))
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

std::vector<std::weak_ptr<BaseObj>> LineOfSight::GetUpSideObstacles() { return upSideObstacles; }
std::vector<std::weak_ptr<BaseObj>> LineOfSight::GetLeftSideObstacles() { return leftSideObstacles; }
std::vector<std::weak_ptr<BaseObj>> LineOfSight::GetDownSideObstacles() { return downSideObstacles; }
std::vector<std::weak_ptr<BaseObj>> LineOfSight::GetRightSideObstacles() { return rightSideObstacles; }
