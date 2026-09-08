#pragma once

#include "entities/BulletCalibre.h"
#include "enums/Author.h"
#include "interfaces/IShootable.h"
#include <memory>
#include <optional>

enum class Direction : char8_t;
struct BulletCalibre;
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
	BulletCalibre& _calibre;

	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<EventSystem> _events{nullptr};

	[[nodiscard]] ObjRectangle GetBulletStartRect() const;

public:
	ShootingBeh(ObjRectangle& rect, Direction& dir, Uuid& uuid, Author& author,
				const std::shared_ptr<BulletPool>& bulletPool, BulletCalibre& calibre,
				const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig);

	[[nodiscard]] Uuid Shot(std::optional<Uuid> uuid) override;
};
