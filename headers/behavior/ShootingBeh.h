#pragma once

#include "entities/BulletCalibre.h"
#include "interfaces/IShootable.h"
#include <functional>
#include <memory>

enum class Direction : char8_t;
struct BulletCalibre;
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
	UPoint& _windowSize;
	std::string& _name;
	std::string& _fraction;
	BulletCalibre& _calibre;

	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};

	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<EventSystem> _events{nullptr};

	[[nodiscard]] float FindMinDistance(const std::vector<std::shared_ptr<BaseObj>>& objects,
										const std::function<float(const std::shared_ptr<BaseObj>&)>& sideDiff) const;

	[[nodiscard]] ObjRectangle GetBulletStartRect() const;

public:
	ShootingBeh(ObjRectangle& rect, Direction& dir, buuid& uuid, UPoint& windowSize, std::string& name,
				std::string& fraction, std::vector<std::shared_ptr<BaseObj>>* allObjects,
				const std::shared_ptr<BulletPool>& bulletPool, BulletCalibre& calibre,
				const std::shared_ptr<EventSystem>& events);

	~ShootingBeh() override;

	[[nodiscard]] buuid Shot(buuid uuid = {}) override;
};
