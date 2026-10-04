#pragma once

#include "entities/BulletCaliber.h"
#include "enums/Author.h"
#include "interfaces/IShootable.h"
#include <memory>
#include <optional>
#include <vector>

enum class Direction : char8_t;
struct BulletCaliber;
struct FPoint;
struct ObjRectangle;
class BaseObj;
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

	//NOTE: rows from the muzzle outwards, times shells abreast
	struct VolleyShape final
	{
		int depth{};
		int width{};
	};

	//NOTE: one shell up to the third tier, as in the original; past it a star adds shells, a row deeper first
	[[nodiscard]] static VolleyShape ShapeOf(unsigned short tier) noexcept;
	[[nodiscard]] ObjRectangle GetBulletStartRect() const;
	//NOTE: the caliber this one shot leaves with - the tank's own is untouched, the spread lives per shell
	[[nodiscard]] BulletCaliber CaliberOfShot(std::optional<unsigned int> damage) const;
	//NOTE: whether a shell could have flown this way - a shell born past a wall would be a shot through it
	[[nodiscard]] bool IsClear(const ObjRectangle& path, const std::vector<std::shared_ptr<BaseObj>>& objects) const;
	[[nodiscard]] ShotResult SpawnShell(const ObjRectangle& rect, std::optional<Uuid> uuid,
										std::optional<unsigned int> damage) const;

public:
	ShootingBeh(ObjRectangle& rect, Direction& dir, Uuid& uuid, Author& author,
				const std::shared_ptr<BulletPool>& bulletPool, BulletCaliber& caliber,
				const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig);

	[[nodiscard]] std::vector<ShotResult> Volley(const std::vector<std::shared_ptr<BaseObj>>& objects) override;
	void Mirror(Uuid uuid, unsigned int damage) override;
};
