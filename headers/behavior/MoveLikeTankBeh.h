#pragma once

#include "Point.h"
#include "entities/ObjRectangle.h"
#include "interfaces/IMoveBeh.h"
#include <boost/uuid/uuid.hpp>
#include <functional>
#include <memory>

class BaseObj;
class Tank;

class MoveLikeTankBeh final : public IMoveBeh
{
	using buuid = boost::uuids::uuid;

	buuid& _uuid;
	ObjRectangle& _rect;
	Direction& _direction;
	float& _speed;
	UPoint& _windowSize;//TODO: subscribe on windows size change
	std::string& _name;
	std::string& _fraction;
	std::vector<std::shared_ptr<BaseObj>>& _touchedObstacles;

	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};

	void HandleBonusPickUp(const std::shared_ptr<BaseObj>& object) const;

	[[nodiscard]] bool IsCanMove(double deltaTime) const override;
	[[nodiscard]] float FindMinDistance(const std::vector<std::shared_ptr<BaseObj>>& objects,
										const std::function<float(const std::shared_ptr<BaseObj>&)>& sideDiff) const;

	[[nodiscard]] bool MoveLeft(double deltaTime) override;
	[[nodiscard]] bool MoveRight(double deltaTime) override;
	[[nodiscard]] bool MoveUp(double deltaTime) override;
	[[nodiscard]] bool MoveDown(double deltaTime) override;

public:
	MoveLikeTankBeh(ObjRectangle& rect, Direction& dir, float& speed, buuid& uuid, UPoint& windowSize,
					std::string& name, std::string& fraction, std::vector<std::shared_ptr<BaseObj>>& touchedObstacles,
					std::vector<std::shared_ptr<BaseObj>>* allObjects);

	~MoveLikeTankBeh() override = default;

	[[nodiscard]] bool Move(double deltaTime) override;

	[[nodiscard]] std::vector<Direction> GetFreePathSides(double deltaTime) const override;
};
