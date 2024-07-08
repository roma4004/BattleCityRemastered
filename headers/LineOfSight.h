#pragma once
#include "BaseObj.h"
#include "Point.h"
#include "utils/ColliderCheck.h"

#include <memory>
#include <vector>

class LineOfSight
{
	std::vector<Rectangle> checkLOS;
	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};

	std::vector<std::weak_ptr<BaseObj>> upSideObstacles{};
	std::vector<std::weak_ptr<BaseObj>> leftSideObstacles{};
	std::vector<std::weak_ptr<BaseObj>> downSideObstacles{};
	std::vector<std::weak_ptr<BaseObj>> rightSideObstacles{};

public:
	LineOfSight(Rectangle shape, UPoint windowSize, FPoint bulletHalf,
	             std::vector<std::shared_ptr<BaseObj>>* allObjects, const BaseObj* excludeSelf);
	~LineOfSight();

	[[nodiscard]] std::vector<std::weak_ptr<BaseObj>> GetUpSideObstacles();
	[[nodiscard]] std::vector<std::weak_ptr<BaseObj>> GetLeftSideObstacles();
	[[nodiscard]] std::vector<std::weak_ptr<BaseObj>> GetDownSideObstacles();
	[[nodiscard]] std::vector<std::weak_ptr<BaseObj>> GetRightSideObstacles();
};
