#pragma once

#include "../BaseObj.h"
#include "../interfaces/IMoveBeh.h"

#include <functional>
#include <memory>
#include <random>

class MoveLikeAIBeh final : public IMoveBeh
{
	std::mt19937 gen;
	std::uniform_int_distribution<> distDirection;

	BaseObj* _selfParent{nullptr};
	std::vector<std::shared_ptr<BaseObj>>* _allObjects;

	void Move(float deltaTime) const override;
	void MoveLeft(float deltaTime) const override;
	void MoveRight(float deltaTime) const override;
	void MoveUp(float deltaTime) const override;
	void MoveDown(float deltaTime) const override;

public:
	MoveLikeAIBeh(BaseObj* selfParent, std::vector<std::shared_ptr<BaseObj>>* allObjects);

	~MoveLikeAIBeh() override = default;

	[[nodiscard]] std::list<std::weak_ptr<BaseObj>> IsCanMove(float deltaTime) const override;
	[[nodiscard]] float FindMinDistance(const std::list<std::weak_ptr<BaseObj>>& objects,
	                                    const std::function<float(const std::shared_ptr<BaseObj>&)>& sideDiff) const;
};
