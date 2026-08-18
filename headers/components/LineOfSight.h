#pragma once

#include "entities/ObjRectangle.h"//NOTE: complete type needed - a std::vector<ObjRectangle> member is instantiated below

#include <memory>
#include <vector>

struct UPoint;
struct FPoint;
class BaseObj;
class GameConfig;

class LineOfSight final
{
	std::vector<ObjRectangle> _lineOfSightBoundaries{};
	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};

	std::vector<std::shared_ptr<BaseObj>> _upSideObstacles{};
	std::vector<std::shared_ptr<BaseObj>> _leftSideObstacles{};
	std::vector<std::shared_ptr<BaseObj>> _downSideObstacles{};
	std::vector<std::shared_ptr<BaseObj>> _rightSideObstacles{};

public:
	LineOfSight(ObjRectangle tankRect, FPoint bulletSize, std::vector<std::shared_ptr<BaseObj>>* allObjects,
				const GameConfig& gameConfig, bool isWaterSkip = true);
	LineOfSight(ObjRectangle tankRect, std::vector<std::shared_ptr<BaseObj>>* allObjects,
				const GameConfig& gameConfig, bool isWaterSkip = true);

	~LineOfSight();

	void CheckLineOfSight(bool isWaterSkip);
	void SortToNearest();

	[[nodiscard]] std::vector<std::shared_ptr<BaseObj>>& GetUpSideObstacles();
	[[nodiscard]] std::vector<std::shared_ptr<BaseObj>>& GetLeftSideObstacles();
	[[nodiscard]] std::vector<std::shared_ptr<BaseObj>>& GetDownSideObstacles();
	[[nodiscard]] std::vector<std::shared_ptr<BaseObj>>& GetRightSideObstacles();
};
