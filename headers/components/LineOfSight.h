#pragma once

#include "geometry/ObjRectangle.h"

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

	//NOTE: raw pointers - a sight is read in the tick it is taken, while the world keeps them alive
	std::vector<BaseObj*> _upSideObstacles{};
	std::vector<BaseObj*> _leftSideObstacles{};
	std::vector<BaseObj*> _downSideObstacles{};
	std::vector<BaseObj*> _rightSideObstacles{};

	void CheckLineOfSight(bool isWaterSkip, const std::vector<std::shared_ptr<BaseObj>>& objects);
	void SortToNearest();

public:
	LineOfSight(ObjRectangle tankRect, FPoint bulletSize, const std::vector<std::shared_ptr<BaseObj>>& objects,
				const GameConfig& gameConfig, bool isWaterSkip = true);
	LineOfSight(ObjRectangle tankRect, const std::vector<std::shared_ptr<BaseObj>>& objects,
				const GameConfig& gameConfig, bool isWaterSkip = true);

	[[nodiscard]] std::vector<BaseObj*>& SideObstacles(Direction dir);
};
