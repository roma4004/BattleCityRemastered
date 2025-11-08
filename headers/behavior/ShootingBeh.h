#pragma once

#include "interfaces/IShootable.h"
#include <functional>
#include <memory>

enum class Direction : char8_t;
struct FPoint;
struct UPoint;
struct ObjRectangle;
class BaseObj;
class EventSystem;
class BulletPool;

class ShootingBeh final : public IShootable
{
	using buuid = boost::uuids::uuid;

	buuid& _uuid;
	ObjRectangle& _rect;
	Direction& _direction;
	float& _speed;
	float& _bulletSpeed;
	int& _bulletDamage;
	double& _bulletDamageRadius;
	int& _tier;
	FPoint& _bulletSize;
	UPoint& _windowSize;
	std::string& _name;
	std::string& _fraction;

	// BaseObj* _selfParent{nullptr};//TODO: replace with shared ptr
	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};

	std::shared_ptr<BulletPool> _bulletPool{nullptr};

	[[nodiscard]] float FindMinDistance(const std::vector<std::shared_ptr<BaseObj>>& objects,
	                                    const std::function<float(const std::shared_ptr<BaseObj>&)>& sideDiff) const;

	[[nodiscard]] ObjRectangle GetBulletStartRect() const;

public:
	ShootingBeh(ObjRectangle& rect, Direction& dir, float& speed, buuid& uuid, float& bulletSpeed, int& bulletDamage,
	            int& tier, double& damageRadius, FPoint& bulletSize, UPoint& windowSize, std::string& name,
	            std::string& fraction, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	            const std::shared_ptr<BulletPool>& bulletPool);

	~ShootingBeh() override;

	[[nodiscard]] buuid Shot(buuid uuid = {}) override;
};
