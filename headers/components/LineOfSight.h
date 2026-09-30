#pragma once

#include "geometry/ObjRectangle.h"//NOTE: complete type needed - a std::vector<ObjRectangle> member is instantiated below

#include <array>
#include <memory>
#include <vector>

enum class Direction : char8_t;
struct UPoint;
struct FPoint;
class BaseObj;
class GameConfig;

class LineOfSight final
{
	//NOTE: up, left, down, right - the order Direction lists them, so a side indexes straight into it
	std::array<ObjRectangle, 4> _lineOfSightBoundaries{};

	std::vector<std::shared_ptr<BaseObj>> _upSideObstacles{};
	std::vector<std::shared_ptr<BaseObj>> _leftSideObstacles{};
	std::vector<std::shared_ptr<BaseObj>> _downSideObstacles{};
	std::vector<std::shared_ptr<BaseObj>> _rightSideObstacles{};

	void CheckLineOfSight(bool isWaterSkip, const std::vector<std::shared_ptr<BaseObj>>& objects);
	void SortToNearest();

public:
	LineOfSight(ObjRectangle tankRect, FPoint bulletSize, const std::vector<std::shared_ptr<BaseObj>>& objects,
				const GameConfig& gameConfig, bool isWaterSkip = true);
	LineOfSight(ObjRectangle tankRect, const std::vector<std::shared_ptr<BaseObj>>& objects,
				const GameConfig& gameConfig, bool isWaterSkip = true);

	[[nodiscard]] std::vector<std::shared_ptr<BaseObj>>& SideObstacles(Direction dir);
};
