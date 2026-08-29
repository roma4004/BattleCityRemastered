#include "components/LineOfSight.h"
#include "geometry/Point.h"
#include "application/GameConfig.h"
#include "entities/obstacles/BushTile.h"
#include "entities/obstacles/WaterTile.h"
#include "enums/Direction.h"
#include "utils/ColliderUtils.h"
#include <algorithm>

//Used for checking line of sight for bullets, so range is bullet width and height 
LineOfSight::LineOfSight(const ObjRectangle tankRect, const FPoint bulletSize,
						 std::vector<std::shared_ptr<BaseObj>>* allObjects, const GameConfig& gameConfig,
						 const bool isWaterSkip)
	: _allObjects{allObjects}
{
	const FPoint tankHalf{.x = tankRect.w / 2.0, .y = tankRect.h / 2.0};

	const FPoint tankUpCenter{.x = tankRect.x + tankHalf.x, .y = tankRect.y};
	const FPoint tankLeftCenter{.x = tankRect.x, .y = tankRect.y + tankHalf.y};
	const FPoint tankDownCenter{.x = tankRect.x + tankHalf.x, .y = tankRect.y + tankRect.h};
	const FPoint tankRightCenter{.x = tankRect.x + tankRect.w, .y = tankRect.y + tankHalf.y};

	const FPoint bulletHalfSize{.x = bulletSize.x / 2.0, .y = bulletSize.y / 2.0};

	const FPoint bulletSpawnPosUp{.x = tankUpCenter.x - bulletHalfSize.x, .y = tankUpCenter.y - bulletSize.y - 1.0};
	const FPoint bulletSpawnPosLeft{.x = tankLeftCenter.x - bulletSize.x - 1.0,
									.y = tankLeftCenter.y - bulletHalfSize.y};
	const FPoint bulletSpawnPosDown{.x = tankDownCenter.x - bulletHalfSize.x, .y = tankDownCenter.y + 1.0};
	const FPoint bulletSpawnPosRight{.x = tankRightCenter.x + 1.0, .y = tankRightCenter.y - bulletHalfSize.y};

	const double sightSizeUp{std::max(0.0, tankRect.y - 1.0)};
	const double sightSizeLeft{std::max(0.0, tankRect.x - 1.0)};
	const double sightSizeDown{static_cast<double>(gameConfig.battlefieldSize.y) - bulletSpawnPosDown.y};
	const double sightSizeRight{static_cast<double>(gameConfig.battlefieldSize.x) - bulletSpawnPosRight.x};

	_lineOfSightBoundaries = std::vector<ObjRectangle>{/*up, left, down, right*/
			{.x = bulletSpawnPosUp.x, .y = 0.0, .w = bulletSize.x, .h = sightSizeUp},
			{.x = 0.0, .y = bulletSpawnPosLeft.y, .w = sightSizeLeft, .h = bulletSize.y},
			{.x = bulletSpawnPosDown.x, .y = bulletSpawnPosDown.y, .w = bulletSize.x, .h = sightSizeDown},
			{.x = bulletSpawnPosRight.x, .y = bulletSpawnPosRight.y, .w = sightSizeRight, .h = bulletSize.y}};

	CheckLineOfSight(isWaterSkip);
}

//Used for checking can tank reach the bonus, so range is tank width and height 
LineOfSight::LineOfSight(const ObjRectangle tankRect, std::vector<std::shared_ptr<BaseObj>>* allObjects,
						 const GameConfig& gameConfig, const bool isWaterSkip)
	: _allObjects{allObjects}
{
	const double sightSizeUp{std::max(0.0, tankRect.y - 1.0)};
	const double sightSizeLeft{std::max(0.0, tankRect.x - 1.0)};
	const double sightSizeDown{static_cast<double>(gameConfig.battlefieldSize.y) - tankRect.y - tankRect.h - 1};
	const double sightSizeRight{static_cast<double>(gameConfig.battlefieldSize.x) - tankRect.x - tankRect.w - 1};

	_lineOfSightBoundaries = std::vector<ObjRectangle>{/*up, left, down, right*/
			{.x = tankRect.x, .y = 0.0, .w = tankRect.w, .h = sightSizeUp},
			{.x = 0.0, .y = tankRect.y, .w = sightSizeLeft, .h = tankRect.h},
			{.x = tankRect.x, .y = tankRect.y + tankRect.h + 1, .w = tankRect.w, .h = sightSizeDown},
			{.x = tankRect.x + tankRect.w + 1, .y = tankRect.y, .w = sightSizeRight, .h = tankRect.h}};

	CheckLineOfSight(isWaterSkip);
}

LineOfSight::~LineOfSight() = default;

void LineOfSight::CheckLineOfSight(const bool isWaterSkip = false)
{
	// parse all seen in Line Of Sight obj
	for (std::shared_ptr<BaseObj>& object: *_allObjects)
	{
		const ObjRectangle& upSideRect = _lineOfSightBoundaries[static_cast<size_t>(Direction::UP)];
		const ObjRectangle& leftSightRect = _lineOfSightBoundaries[static_cast<size_t>(Direction::LEFT)];
		const ObjRectangle& downSideRect = _lineOfSightBoundaries[static_cast<size_t>(Direction::DOWN)];
		const ObjRectangle& rightSightRect = _lineOfSightBoundaries[static_cast<size_t>(Direction::RIGHT)];

		// NOTE: tank can't pass water (until pickup BonusShip), so we skip water when find opponent to shoot,
		// but for searching for bonuses, we should not skip, to avoid trying to move through water.
		const bool isWater = dynamic_cast<WaterTile*>(object.get()) != nullptr;
		const bool isBush = dynamic_cast<BushTile*>(object.get()) != nullptr;
		const bool isPenetrable = object->GetIsPenetrable();
		if (isBush
			|| (isWater && !isWaterSkip)
			|| !isPenetrable)
		{
			if (ColliderUtils::IsCollide(upSideRect, object->GetRect()))
			{
				_upSideObstacles.emplace_back(object);
			}

			if (ColliderUtils::IsCollide(leftSightRect, object->GetRect()))
			{
				_leftSideObstacles.emplace_back(object);
			}

			if (ColliderUtils::IsCollide(downSideRect, object->GetRect()))
			{
				_downSideObstacles.emplace_back(object);
			}

			if (ColliderUtils::IsCollide(rightSightRect, object->GetRect()))
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
