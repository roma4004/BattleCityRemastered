#pragma once

#include "interfaces/IMoveBeh.h"
#include <functional>
#include <memory>

class BaseObj;
class Tank;

class MoveLikeTankBeh final : public IMoveBeh
{
	BaseObj* _selfParent{nullptr};//TODO: replace with weak_ptr
	std::vector<std::shared_ptr<BaseObj>>* _allObjects;

	static void HandleBonusPickUp(const std::shared_ptr<BaseObj>& object, const Tank* tank);

	[[nodiscard]] std::vector<std::shared_ptr<BaseObj>> IsCanMove(float deltaTime) const override;
	[[nodiscard]] float FindMinDistance(const std::vector<std::shared_ptr<BaseObj>>& objects,
	                                    const std::function<float(const std::shared_ptr<BaseObj>&)>& sideDiff) const;

	[[nodiscard]] bool MoveLeft(float deltaTime) const override;
	[[nodiscard]] bool MoveRight(float deltaTime) const override;
	[[nodiscard]] bool MoveUp(float deltaTime) const override;
	[[nodiscard]] bool MoveDown(float deltaTime) const override;

public:
	MoveLikeTankBeh(BaseObj* selfParent, std::vector<std::shared_ptr<BaseObj>>* allObjects);

	~MoveLikeTankBeh() override = default;

	[[nodiscard]] bool Move(float deltaTime) const override;

	[[nodiscard]] std::vector<Direction> GetFreePathSides(float deltaTime) const override;
};
