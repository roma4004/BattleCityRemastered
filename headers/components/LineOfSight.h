#pragma once

#include "geometry/ObjRectangle.h"//NOTE: complete type needed - a std::vector<ObjRectangle> member is instantiated below

#include <memory>
#include <vector>

struct UPoint;
struct FPoint;
class BaseObj;
class GameConfig;

class LineOfSight final
{
	std::vector<ObjRectangle> _lineOfSightBoundaries{};

	std::vector<std::shared_ptr<BaseObj>> _upSideObstacles{};
	std::vector<std::shared_ptr<BaseObj>> _leftSideObstacles{};
	std::vector<std::shared_ptr<BaseObj>> _downSideObstacles{};
	std::vector<std::shared_ptr<BaseObj>> _rightSideObstacles{};

	void CheckLineOfSight(bool isWaterSkip, const std::vector<std::shared_ptr<BaseObj>>& objects);

public:
	LineOfSight(ObjRectangle tankRect, FPoint bulletSize, const std::vector<std::shared_ptr<BaseObj>>& objects,
				const GameConfig& gameConfig, bool isWaterSkip = true);
	LineOfSight(ObjRectangle tankRect, const std::vector<std::shared_ptr<BaseObj>>& objects,
				const GameConfig& gameConfig, bool isWaterSkip = true);

	~LineOfSight();

	void SortToNearest();

	[[nodiscard]] std::vector<std::shared_ptr<BaseObj>>& GetUpSideObstacles();
	[[nodiscard]] std::vector<std::shared_ptr<BaseObj>>& GetLeftSideObstacles();
	[[nodiscard]] std::vector<std::shared_ptr<BaseObj>>& GetDownSideObstacles();
	[[nodiscard]] std::vector<std::shared_ptr<BaseObj>>& GetRightSideObstacles();
};
