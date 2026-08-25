#pragma once

#include "geometry/ObjRectangle.h"
#include "interfaces/IMoveBeh.h"
#include "utils/Uuid.h"
#include <functional>
#include <memory>
#include <optional>
#include <string>

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
	float& _speed;
	BonusEffectProperty& _effects;
	float _upVelocity{};
	float _leftVelocity{};
	float _downVelocity{};
	float _rightVelocity{};
	float _driftMultiplicator{1.5f};
	std::string& _name;
	Faction& _faction;
	const GameConfig& _gameConfig;

	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};

	[[nodiscard]] bool IsBlocking(const std::shared_ptr<BaseObj>& object, const ObjRectangle& nextPosRect) const;
	[[nodiscard]] bool IsCanMove(double deltaTime, Direction dir) const override;
	[[nodiscard]] std::vector<std::shared_ptr<BaseObj>> GetTouchedObjects(double deltaTime) const;
	[[nodiscard]] ObjRectangle GetNextPosRect(double deltaTime, Direction dir) const;
	[[nodiscard]] float FindMinDistance(const std::vector<std::shared_ptr<BaseObj>>& objects,
										const std::function<float(const std::shared_ptr<BaseObj>&)>& sideDiff) const;

protected:
	[[nodiscard]] bool MoveUp(double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions) override;
	[[nodiscard]] bool MoveLeft(double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions) override;
	[[nodiscard]] bool MoveDown(double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions) override;
	[[nodiscard]] bool MoveRight(double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions) override;

public:
	MoveLikeTankBeh(ObjRectangle& rect, Direction& dir, float& speed, Uuid& uuid, std::string& name,
					Faction& faction, std::vector<std::shared_ptr<BaseObj>>* allObjects,
					BonusEffectProperty& effects, const GameConfig& gameConfig);

	~MoveLikeTankBeh() override = default;

	[[nodiscard]]
	bool Move(Direction dir, double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions) override;
	[[nodiscard]] bool ApplyMoveVelocity(double deltaTime);
	void ResetVelocity();
	[[nodiscard]] std::vector<Direction> GetFreePathSides(
			double deltaTime, std::optional<Direction> excludeDirection) const override;
};
