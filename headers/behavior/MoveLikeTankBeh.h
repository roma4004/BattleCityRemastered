#pragma once

#include "Point.h"
#include "entities/ObjRectangle.h"
#include "interfaces/IMoveBeh.h"
#include <boost/uuid/uuid.hpp>
#include <functional>
#include <memory>

struct BonusEffectProperty;
class BaseObj;
class Tank;

class MoveLikeTankBeh final : public IMoveBeh
{
	using buuid = boost::uuids::uuid;

	buuid& _uuid;
	ObjRectangle& _rect;
	Direction& _direction;
	float& _speed;
	BonusEffectProperty& _effects;
	float _upVelocity{};
	float _leftVelocity{};
	float _downVelocity{};
	float _rightVelocity{};
	float _driftMultiplicator{1.5f};
	UPoint& _windowSize;
	std::string& _name;
	std::string& _fraction;

	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};

	[[nodiscard]] bool IsCanMove(double deltaTime, Direction dir) const override;
	[[nodiscard]] std::vector<std::shared_ptr<BaseObj>> GetTouchedObjects(double deltaTime) const;
	[[nodiscard]] ObjRectangle GetNextPosRect(double deltaTime, Direction dir) const;
	[[nodiscard]] float FindMinDistance(const std::vector<std::shared_ptr<BaseObj>>& objects,
										const std::function<float(const std::shared_ptr<BaseObj>&)>& sideDiff) const;

protected:
	[[nodiscard]] bool MoveLeft(double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions) override;
	[[nodiscard]] bool MoveRight(double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions) override;
	[[nodiscard]] bool MoveUp(double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions) override;
	[[nodiscard]] bool MoveDown(double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions) override;

public:
	MoveLikeTankBeh(ObjRectangle& rect, Direction& dir, float& speed, buuid& uuid, UPoint& windowSize,
					std::string& name, std::string& fraction, std::vector<std::shared_ptr<BaseObj>>* allObjects,
					BonusEffectProperty& effects);

	~MoveLikeTankBeh() override = default;

	[[nodiscard]]
	bool Move(Direction dir, double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions) override;
	[[nodiscard]] bool ApplyMoveVelocity(double deltaTime) override;
	void ResetVelocity() override;
};
