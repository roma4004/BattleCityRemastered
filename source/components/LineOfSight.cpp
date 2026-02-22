#include "components/LineOfSight.h"
#include "Point.h"
#include "entities/obstacles/WaterTile.h"
#include "enums/Direction.h"
#include "utils/ColliderUtils.h"
#include <algorithm>

LineOfSight::LineOfSight(const ObjRectangle tankRect, const UPoint& windowSize, const FPoint bulletSize,
						 std::vector<std::shared_ptr<BaseObj>>* allObjects, const BaseObj* excludeSelf,
						 const bool isWaterSkip)
	: _allObjects{allObjects}
{
	const FPoint fWindowSize = {.x = static_cast<float>(windowSize.x), .y = static_cast<float>(windowSize.y)};
	const FPoint tankHalf = {.x = tankRect.w / 2.f, .y = tankRect.h / 2.f};
	const FPoint tankCenter = {.x = tankRect.x + tankHalf.x, .y = tankRect.y + tankHalf.y};
	const float tankDownY = {tankRect.y + tankRect.h};
	const float tankRightX = {tankRect.x + tankRect.w};
	const FPoint bulletSpawnPos = {.x = tankCenter.x - bulletSize.x, .y = tankCenter.y - bulletSize.y};
	const FPoint bulletHalfSize = {.x = bulletSize.x / 2, .y = bulletSize.y / 2};
	const FPoint sightSize = {.x = fWindowSize.x - tankRightX, .y = fWindowSize.y - tankDownY};

	_lineOfSightBoundaries = std::vector<ObjRectangle>{
			/*up, left, down, right*///TODO: align to not needed exclude self
			{.x = bulletSpawnPos.x - bulletHalfSize.x, .y = 0.f, .w = bulletSize.x, .h = tankRect.y},
			{.x = 0.f, .y = bulletSpawnPos.y - bulletHalfSize.x, .w = tankRect.x, .h = bulletSize.y},
			{.x = bulletSpawnPos.x - bulletHalfSize.x, .y = tankDownY, .w = bulletSize.x, .h = sightSize.y},
			{.x = tankRightX, .y = bulletSpawnPos.y - bulletHalfSize.x, .w = sightSize.x, .h = bulletSize.y}};

	CheckLineOfSight(excludeSelf, isWaterSkip);
}

LineOfSight::LineOfSight(const ObjRectangle tankRect, const UPoint& windowSize,
						 std::vector<std::shared_ptr<BaseObj>>* allObjects, const BaseObj* excludeSelf,
						 const bool isWaterSkip)
	: _allObjects{allObjects}
{
	const float tankDownY = {tankRect.y + tankRect.h};
	const float tankRightX = {tankRect.x + tankRect.w};
	const FPoint fWindowSize = {.x = static_cast<float>(windowSize.x), .y = static_cast<float>(windowSize.y)};
	const FPoint sightSize = {.x = fWindowSize.x - tankRightX, .y = fWindowSize.y - tankDownY};

	_lineOfSightBoundaries =
			std::vector<ObjRectangle>{/*up, left, down, right*/
					{.x = tankRect.x, .y = 0.f, .w = tankRect.w, .h = tankRect.y},
					{.x = 0.f, .y = tankRect.y, .w = tankRect.x, .h = tankRect.h},
					{.x = tankRect.x, .y = tankDownY, .w = tankRect.w, .h = sightSize.y},
					{.x = tankRightX, .y = tankRect.y, .w = sightSize.x, .h = tankRect.h}};

	CheckLineOfSight(excludeSelf, isWaterSkip);
}

LineOfSight::~LineOfSight() = default;

void LineOfSight::CheckLineOfSight(const BaseObj* excludeSelf, const bool isWaterSkip = false)
{
	// parse all seen in Line Of Sight obj
	for (std::shared_ptr<BaseObj>& object: *_allObjects)
	{
		if (excludeSelf->GetUuid() == object->GetUuid())
		{
			continue;
		}

		// tank cannot pass water, so we need to skip water when we find enemy to shoot
		// but when we search for bonus, we should not skip water to avoid moving to bonus through water.
		const bool isWater = dynamic_cast<WaterTile*>(object.get());
		const bool isPenetrable = object->GetIsPenetrable();
		if (!object->GetIsPassable() && (!isPenetrable || (isWater && !isWaterSkip)))
		{
			if (ColliderUtils::IsCollide(_lineOfSightBoundaries[static_cast<int>(Direction::UP)], object->GetRect()))
			{
				_upSideObstacles.emplace_back(object);
			}

			if (ColliderUtils::IsCollide(_lineOfSightBoundaries[static_cast<int>(Direction::LEFT)], object->GetRect()))
			{
				_leftSideObstacles.emplace_back(object);
			}

			if (ColliderUtils::IsCollide(_lineOfSightBoundaries[static_cast<int>(Direction::DOWN)], object->GetRect()))
			{
				_downSideObstacles.emplace_back(object);
			}

			if (ColliderUtils::IsCollide(_lineOfSightBoundaries[static_cast<int>(Direction::RIGHT)], object->GetRect()))
			{
				_rightSideObstacles.emplace_back(object);
			}
		}
	}

	SortToNearest();
}

void LineOfSight::SortToNearest()
{
	std::ranges::sort(_upSideObstacles, [](const std::shared_ptr<BaseObj>& a, const std::shared_ptr<BaseObj>& b)
	{
		if (!a)
		{
			return false;
		}

		if (!b)
		{
			return true;
		}

		return a->GetPos().y > b->GetPos().y;
	});

	std::ranges::sort(_leftSideObstacles, [](const std::shared_ptr<BaseObj>& a, const std::shared_ptr<BaseObj>& b)
	{
		if (!a)
		{
			return false;
		}

		if (!b)
		{
			return true;
		}

		return a->GetPos().x > b->GetPos().x;
	});

	std::ranges::sort(_downSideObstacles, [](const std::shared_ptr<BaseObj>& a, const std::shared_ptr<BaseObj>& b)
	{
		if (!a)
		{
			return false;
		}

		if (!b)
		{
			return true;
		}

		return a->GetPos().y < b->GetPos().y;
	});

	std::ranges::sort(_rightSideObstacles, [](const std::shared_ptr<BaseObj>& a, const std::shared_ptr<BaseObj>& b)
	{
		if (!a)
		{
			return false;
		}

		if (!b)
		{
			return true;
		}

		return a->GetPos().x < b->GetPos().x;
	});
}

std::vector<std::shared_ptr<BaseObj>>& LineOfSight::GetUpSideObstacles() { return _upSideObstacles; }
std::vector<std::shared_ptr<BaseObj>>& LineOfSight::GetLeftSideObstacles() { return _leftSideObstacles; }
std::vector<std::shared_ptr<BaseObj>>& LineOfSight::GetDownSideObstacles() { return _downSideObstacles; }
std::vector<std::shared_ptr<BaseObj>>& LineOfSight::GetRightSideObstacles() { return _rightSideObstacles; }
