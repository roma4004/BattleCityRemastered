#pragma once

#include "BaseObj.h"
#include "interfaces/IShootable.h"

#include <functional>
#include <memory>

class EventSystem;
class BulletPool;

class ShootingBeh : public IShootable
{
	BaseObj* _selfParent{nullptr};
	int* _windowBuffer;
	std::vector<std::shared_ptr<BaseObj>>* _allObjects;
	std::shared_ptr<EventSystem> _events;

	std::shared_ptr<BulletPool> _bulletPool;

public:
	ShootingBeh(BaseObj* selfParent, int* windowBuffer, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	            std::shared_ptr<EventSystem> events, std::shared_ptr<BulletPool> bulletPool);

	~ShootingBeh() override;

	float FindMinDistance(const std::list<std::weak_ptr<BaseObj>>& objects,
	                      const std::function<float(const std::shared_ptr<BaseObj>&)>& sideDiff) const;

	[[nodiscard]] ObjRectangle GetBulletStartRect() const;

	void Shot() const override;
};
