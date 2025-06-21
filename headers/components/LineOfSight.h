#pragma once

#include <memory>
#include <vector>

struct ObjRectangle;
struct UPoint;
struct FPoint;
class BaseObj;

class LineOfSight final
{
	std::vector<ObjRectangle> _lineOfSightRect;
	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};

	std::vector<std::shared_ptr<BaseObj>> _upSideObstacles{};
	std::vector<std::shared_ptr<BaseObj>> _leftSideObstacles{};
	std::vector<std::shared_ptr<BaseObj>> _downSideObstacles{};
	std::vector<std::shared_ptr<BaseObj>> _rightSideObstacles{};

public:
	LineOfSight(ObjRectangle tankShape, const UPoint& windowSize, FPoint bulletSize,
	            std::vector<std::shared_ptr<BaseObj>>* allObjects, const BaseObj* excludeSelf,
	            bool isWaterSkip = true);
	LineOfSight(ObjRectangle tankShape, const UPoint& windowSize,
	            std::vector<std::shared_ptr<BaseObj>>* allObjects, const BaseObj* excludeSelf,
	            bool isWaterSkip = true);


	~LineOfSight();

	void CheckLineOfSight(const BaseObj* excludeSelf, bool isWaterSkip);
	void SortToNearest();

	[[nodiscard]] std::vector<std::shared_ptr<BaseObj>>& GetUpSideObstacles();
	[[nodiscard]] std::vector<std::shared_ptr<BaseObj>>& GetLeftSideObstacles();
	[[nodiscard]] std::vector<std::shared_ptr<BaseObj>>& GetDownSideObstacles();
	[[nodiscard]] std::vector<std::shared_ptr<BaseObj>>& GetRightSideObstacles();
};
