#pragma once

#include "BaseObj.h"
#include "EventSystem.h"
#include "interfaces/IMoveBeh.h"

#include <functional>
#include <memory>

class MoveLikeBulletBeh : public IMoveBeh
{
	BaseObj* _selfParent{nullptr};
	std::vector<std::shared_ptr<BaseObj>>* _allObjects;
	std::shared_ptr<EventSystem> _events;

public:
	MoveLikeBulletBeh(BaseObj* parent, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	                  std::shared_ptr<EventSystem> events);

	~MoveLikeBulletBeh() override = default;

	void Move(float deltaTime) const override;
	void MoveLeft(float deltaTime) const override;
	void MoveRight(float deltaTime) const override;
	void MoveUp(float deltaTime) const override;
	void MoveDown(float deltaTime) const override;

private:
	void CheckCircleAoE(FPoint blowCenter, std::list<std::weak_ptr<BaseObj>>& aoeList) const;
	void DealDamage(const std::list<std::weak_ptr<BaseObj>>& objectList) const;
	[[nodiscard]] std::list<std::weak_ptr<BaseObj>> IsCanMove(float deltaTime) const override;
};
