#pragma once

#include "../BaseObj.h"
#include "../interfaces/IMoveBeh.h"

#include <functional>
#include <memory>

class Tank;

class MoveLikeTankBeh final : public IMoveBeh
{
	BaseObj* _selfParent{nullptr};
	std::vector<std::shared_ptr<BaseObj>>* _allObjects;

	static void HandleBonusPickUp(const std::weak_ptr<BaseObj>& object, const Tank* tank);

public:
	MoveLikeTankBeh(BaseObj* selfParent, std::vector<std::shared_ptr<BaseObj>>* allObjects);

	~MoveLikeTankBeh() override = default;

	[[nodiscard]] std::vector<std::weak_ptr<BaseObj>> IsCanMove(float deltaTime) const override;
	float FindMinDistance(const std::vector<std::weak_ptr<BaseObj>>& objects,
	                      const std::function<float(const std::shared_ptr<BaseObj>&)>& sideDiff) const;

	void Move(float deltaTime) const override;
	void MoveLeft(float deltaTime) const override;
	void MoveRight(float deltaTime) const override;
	void MoveUp(float deltaTime) const override;
	void MoveDown(float deltaTime) const override;
};
