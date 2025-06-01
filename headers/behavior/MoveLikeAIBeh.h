#pragma once

#include "../interfaces/IMoveBeh.h"

#include <functional>
#include <memory>
#include <random>

class BaseObj;
class Tank;

class MoveLikeAIBeh final : public IMoveBeh
{
	std::mt19937 gen;
	std::uniform_int_distribution<> distDirection;

	BaseObj* _selfParent{nullptr};
	std::vector<std::shared_ptr<BaseObj>>* _allObjects;

	[[nodiscard]] bool Move(float deltaTime) const override;
	[[nodiscard]] bool MoveLeft(float deltaTime) const override;
	[[nodiscard]] bool MoveRight(float deltaTime) const override;
	[[nodiscard]] bool MoveUp(float deltaTime) const override;
	[[nodiscard]] bool MoveDown(float deltaTime) const override;

	static void HandleBonusPickUp(const std::shared_ptr<BaseObj>& object, const Tank* tank);

public:
	MoveLikeAIBeh(BaseObj* selfParent, std::vector<std::shared_ptr<BaseObj>>* allObjects);

	~MoveLikeAIBeh() override = default;

	[[nodiscard]] std::vector<std::shared_ptr<BaseObj>> IsCanMove(float deltaTime) const override;
	[[nodiscard]] float FindMinDistance(const std::vector<std::shared_ptr<BaseObj>>& objects,
	                                    const std::function<float(const std::shared_ptr<BaseObj>&)>& sideDiff) const;
};
