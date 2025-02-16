#pragma once

#include <memory>
#include <vector>

struct ObjRectangle;
struct UPoint;
struct FPoint;
class BaseObj;

class LineOfSight final
{
	std::vector<ObjRectangle> checkLOS;
	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};

	std::vector<std::weak_ptr<BaseObj>> upSideObstacles{};
	std::vector<std::weak_ptr<BaseObj>> leftSideObstacles{};
	std::vector<std::weak_ptr<BaseObj>> downSideObstacles{};
	std::vector<std::weak_ptr<BaseObj>> rightSideObstacles{};

public:
	LineOfSight(ObjRectangle shape, const UPoint& windowSize, FPoint bulletHalf,
	            std::vector<std::shared_ptr<BaseObj>>* allObjects, const BaseObj* excludeSelf);
	~LineOfSight();

	[[nodiscard]] std::vector<std::weak_ptr<BaseObj>> GetUpSideObstacles();
	[[nodiscard]] std::vector<std::weak_ptr<BaseObj>> GetLeftSideObstacles();
	[[nodiscard]] std::vector<std::weak_ptr<BaseObj>> GetDownSideObstacles();
	[[nodiscard]] std::vector<std::weak_ptr<BaseObj>> GetRightSideObstacles();
};
