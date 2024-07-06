#pragma once

#include "BaseObj.h"
#include "interfaces/IMoveBeh.h"

#include <functional>
#include <memory>

class MoveLikeTankBeh : public IMoveBeh
{
	BaseObj* _selfParent{nullptr};
	std::vector<std::shared_ptr<BaseObj>>* _allObjects;

public:
	MoveLikeTankBeh(BaseObj* selfParent, std::vector<std::shared_ptr<BaseObj>>* allObjects);

	~MoveLikeTankBeh() override = default;

	static bool IsCollideWith(const Rectangle& r1, const Rectangle& r2);
	[[nodiscard]] std::list<std::weak_ptr<BaseObj>> IsCanMove(float deltaTime) const override;
	float FindMinDistance(const std::list<std::weak_ptr<BaseObj>>& objects,
	                      const std::function<float(const std::shared_ptr<BaseObj>&)>& sideDiff) const;

	void Move(float deltaTime) const override;
	void MoveLeft(float deltaTime) const override;
	void MoveRight(float deltaTime) const override;
	void MoveUp(float deltaTime) const override;
	void MoveDown(float deltaTime) const override;
};
