#pragma once

#include "entities/BulletCaliber.h"
#include "enums/Author.h"
#include "interfaces/IShootable.h"
#include <memory>
#include <optional>

enum class Direction : char8_t;
struct BulletCaliber;
struct FPoint;
struct ObjRectangle;
class GameConfig;
class EventSystem;
class BulletPool;

class ShootingBeh final : public IShootable
{
	Uuid& _uuid;
	ObjRectangle& _rect;
	Direction& _direction;
	const GameConfig& _gameConfig;
	Author& _author;
	BulletCaliber& _caliber;

	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<EventSystem> _events{nullptr};

	[[nodiscard]] ObjRectangle GetBulletStartRect() const;
	//NOTE: the caliber this one shot leaves with - the tank's own is untouched, the spread lives per shell
	[[nodiscard]] BulletCaliber CaliberOfShot(std::optional<unsigned int> damage) const;

public:
	ShootingBeh(ObjRectangle& rect, Direction& dir, Uuid& uuid, Author& author,
				const std::shared_ptr<BulletPool>& bulletPool, BulletCaliber& caliber,
				const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig);

	[[nodiscard]] ShotResult Shot(std::optional<Uuid> uuid, std::optional<unsigned int> damage) override;
};
