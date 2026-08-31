#pragma once

#include "geometry/ObjRectangle.h"
#include "interfaces/IMoveBeh.h"
#include "utils/Uuid.h"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

enum class Faction : char8_t;

struct BonusEffectProperty;
class BaseObj;
class Tank;
class GameConfig;

class MoveLikeTankBeh final : public IMoveBeh
{
	Uuid& _uuid;
	ObjRectangle& _rect;
	Direction& _direction;
	double& _speed;
	BonusEffectProperty& _effects;
	double _upVelocity{};
	double _leftVelocity{};
	double _downVelocity{};
	double _rightVelocity{};
	double _driftMultiplicator{1.5};
	const GameConfig& _gameConfig;

	[[nodiscard]] bool IsBlocking(const std::shared_ptr<BaseObj>& object, const ObjRectangle& nextPosRect) const;
	[[nodiscard]] bool IsCanMove(double deltaTime, Direction dir,
								 const std::vector<std::shared_ptr<BaseObj>>& objects) const override;
	[[nodiscard]] std::vector<std::shared_ptr<BaseObj>> GetTouchedObjects(
			double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects) const;
	[[nodiscard]] ObjRectangle GetNextPosRect(double deltaTime, Direction dir) const;
	[[nodiscard]] double FindMinDistance(const std::vector<std::shared_ptr<BaseObj>>& objects,
										 const std::function<double(const std::shared_ptr<BaseObj>&)>& sideDiff) const;

protected:
	[[nodiscard]] bool MoveUp(double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects,
							  std::vector<std::shared_ptr<BaseObj>>& outCollisions) override;
	[[nodiscard]] bool MoveLeft(double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects,
								std::vector<std::shared_ptr<BaseObj>>& outCollisions) override;
	[[nodiscard]] bool MoveDown(double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects,
								std::vector<std::shared_ptr<BaseObj>>& outCollisions) override;
	[[nodiscard]] bool MoveRight(double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects,
								 std::vector<std::shared_ptr<BaseObj>>& outCollisions) override;

public:
	MoveLikeTankBeh(ObjRectangle& rect, Direction& dir, double& speed, Uuid& uuid, BonusEffectProperty& effects,
					const GameConfig& gameConfig);

	~MoveLikeTankBeh() override = default;

	[[nodiscard]]
	bool Move(Direction dir, double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects,
			  std::vector<std::shared_ptr<BaseObj>>& outCollisions) override;
	[[nodiscard]] bool ApplyMoveVelocity(double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects);
	void ResetVelocity();
	[[nodiscard]] std::vector<Direction> GetFreePathSides(
			double deltaTime, std::optional<Direction> excludeDirection,
			const std::vector<std::shared_ptr<BaseObj>>& objects) const override;
};
