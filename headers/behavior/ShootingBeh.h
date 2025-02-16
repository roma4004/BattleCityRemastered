#pragma once

#include "../interfaces/IShootable.h"

#include <functional>
#include <memory>

struct ObjRectangle;
class BaseObj;
class EventSystem;
class BulletPool;

class ShootingBeh final : public IShootable
{
	BaseObj* _selfParent{nullptr};
	std::vector<std::shared_ptr<BaseObj>>* _allObjects;
	std::shared_ptr<EventSystem> _events{nullptr};

	std::shared_ptr<BulletPool> _bulletPool{nullptr};

	[[nodiscard]] float FindMinDistance(const std::vector<std::weak_ptr<BaseObj>>& objects,
	                                    const std::function<float(const std::shared_ptr<BaseObj>&)>& sideDiff) const;

	[[nodiscard]] ObjRectangle GetBulletStartRect() const;

public:
	ShootingBeh(BaseObj* selfParent, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	            std::shared_ptr<EventSystem> events, std::shared_ptr<BulletPool> bulletPool);

	~ShootingBeh() override;

	void Shot() const override;
};
