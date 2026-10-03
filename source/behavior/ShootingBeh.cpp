#include "behavior/ShootingBeh.h"
#include "utils/Log.h"
#include "utils/UuidUtils.h"
#include "application/GameConfig.h"
#include "geometry/Point.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/events/SpawnEvents.h"
#include "entities/BulletCaliber.h"
#include "utils/DirectionUtils.h"
#include "utils/MathUtils.h"
#include "utils/RandUtils.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/BulletResetProperty.h"
#include "entities/pawns/Tank.h"
#include "enums/Direction.h"
#include <algorithm>
#include <memory>
#include <optional>
#include <random>

ShootingBeh::ShootingBeh(ObjRectangle& rect, Direction& dir, Uuid& uuid, Author& author,
						 const std::shared_ptr<BulletPool>& bulletPool,
						 BulletCaliber& caliber, const std::shared_ptr<EventSystem>& events,
						 const GameConfig& gameConfig)
	: _uuid{uuid}
	, _rect{rect}
	, _direction{dir}
	, _gameConfig{gameConfig}
	, _author{author}
	, _caliber{caliber}
	, _bulletPool{bulletPool}
	, _events{events} {}

//NOTE: comes back at {-1, -1} when the muzzle would land off the field
ObjRectangle ShootingBeh::GetBulletStartRect() const
{
	const FPoint tankHalf{.x = _rect.w / 2.0, .y = _rect.h / 2.0};
	const FPoint tankPos{.x = _rect.x, .y = _rect.y};
	const double tankRightX{_rect.Right()};
	const double tankBottomY{_rect.Bottom()};
	const FPoint tankCenter{.x = tankPos.x + tankHalf.x, .y = tankPos.y + tankHalf.y};

	//NOTE: the caliber describes the shell flying upwards, so a sideways shot wears the same box turned
	const auto [bulletWidth, bulletHeight]{DirectionUtils::SizeFacing(_caliber.size, _direction)};
	const FPoint bulletHalf{.x = bulletWidth / 2.0, .y = bulletHeight / 2.0};
	ObjRectangle bulletRect{.x = -1, .y = -1, .w = bulletWidth, .h = bulletHeight};

	if (const Direction dir{_direction};
		dir == Direction::UP && tankPos.y - bulletHeight >= 0.0)
	{
		bulletRect.x = tankCenter.x - bulletHalf.x;
		bulletRect.y = tankPos.y - bulletHeight - 1;
	}
	else if (dir == Direction::LEFT && tankPos.x - bulletWidth >= 0.0)
	{
		bulletRect.x = tankPos.x - bulletWidth - 1;
		bulletRect.y = tankCenter.y - bulletHalf.y;
	}
	else if (dir == Direction::DOWN && tankBottomY + bulletHeight <= static_cast<double>(_gameConfig.battlefieldSize.y))
	{
		bulletRect.x = tankCenter.x - bulletHalf.x;
		bulletRect.y = tankBottomY + 1;
	}
	else if (dir == Direction::RIGHT && tankRightX + bulletWidth <= static_cast<double>(_gameConfig.battlefieldSize.x))
	{
		bulletRect.x = tankRightX + 1;
		bulletRect.y = tankCenter.y - bulletHalf.y;
	}

	return bulletRect;
}

//NOTE: rolled per shot and by the authority alone - a client is handed the number with the shot
BulletCaliber ShootingBeh::CaliberOfShot(const std::optional<unsigned int> damage) const
{
	BulletCaliber shot{_caliber};
	if (damage)
	{
		shot.damage = *damage;

		return shot;
	}

	if (const double spread{_gameConfig.bulletDamageSpread};
		spread > 0.0 && !_gameConfig.IsClient())
	{
		const double base{static_cast<double>(_caliber.damage)};
		const auto lowest{MathUtils::RoundTo<unsigned int>(std::max(base * (1.0 - spread), 1.0))};
		const auto highest{MathUtils::RoundTo<unsigned int>(base * (1.0 + spread))};
		shot.damage = RandUtils::GetRandNumber(std::uniform_int_distribution{lowest, highest});
	}

	return shot;
}

ShotResult ShootingBeh::Shot(const std::optional<Uuid> uuid, const std::optional<unsigned int> damage)
{
	const ObjRectangle rect{GetBulletStartRect()};
	if (rect.x < 0.0 || rect.y < 0.0)
	{
		return ShotResult{};
	}

	const BulletCaliber caliber{CaliberOfShot(damage)};

	const BulletResetProperty bulletResetProperty{
			.rect = rect,
			.dir = _direction,
			.health = static_cast<int>(_caliber.damage),
			.author = _author,
			.authorUuid = _uuid,
			.caliber = caliber,
	};

	const std::shared_ptr<Bullet> bullet{_bulletPool->SpawnBullet(bulletResetProperty, uuid)};

	Log::Detail("bullet spawned uuid " + UuidUtils::GetStringUuid(bullet->GetUuid()));

	_events->EmitEvent(AddToSpawnQueueEvent{.obj = bullet});

	return ShotResult{.uuid = bullet->GetUuid(), .damage = caliber.damage};
}
