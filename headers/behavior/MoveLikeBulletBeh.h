#pragma once

#include "../interfaces/IMoveBeh.h"

#include <functional>
#include <memory>

struct FPoint;
class EventSystem;

class MoveLikeBulletBeh final : public IMoveBeh
{
	BaseObj* _selfParent{nullptr};
	std::vector<std::shared_ptr<BaseObj>>* _allObjects;
	std::shared_ptr<EventSystem> _events{nullptr};

	void CheckCircleAoE(FPoint blowCenter, std::vector<std::weak_ptr<BaseObj>>& aoeList) const;
	void DealDamage(const std::vector<std::weak_ptr<BaseObj>>& objectList) const;
	[[nodiscard]] std::vector<std::weak_ptr<BaseObj>> IsCanMove(float deltaTime) const override;

	void Move(float deltaTime) const override;
	void MoveLeft(float deltaTime) const override;
	void MoveRight(float deltaTime) const override;
	void MoveUp(float deltaTime) const override;
	void MoveDown(float deltaTime) const override;

public:
	MoveLikeBulletBeh(BaseObj* parent, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	                  std::shared_ptr<EventSystem> events);

	~MoveLikeBulletBeh() override = default;
};
