#include "behavior/ShootingBeh.h"
#include "utils/Log.h"
#include "utils/UuidUtils.h"
#include "application/GameConfig.h"
#include "geometry/Point.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/events/SpawnEvents.h"
#include "entities/BulletCalibre.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/BulletResetProperty.h"
#include "entities/pawns/Tank.h"
#include "enums/Direction.h"
#include <memory>
#include <optional>

ShootingBeh::ShootingBeh(ObjRectangle& rect, Direction& dir, Uuid& uuid, const Author author,
						 const std::shared_ptr<BulletPool>& bulletPool,
						 BulletCalibre& calibre, const std::shared_ptr<EventSystem>& events,
						 const GameConfig& gameConfig)
	: _uuid{uuid}
	, _rect{rect}
	, _direction{dir}
	, _gameConfig{gameConfig}
	, _author{author}
	, _calibre{calibre}
	, _bulletPool{bulletPool}
	, _events{events} {}

ShootingBeh::~ShootingBeh() = default;

//Note: {-1.f, -1.f} this is try shooting outside screen
ObjRectangle ShootingBeh::GetBulletStartRect() const
{
	const FPoint tankHalf{.x = _rect.w / 2.0, .y = _rect.h / 2.0};
	const FPoint tankPos{.x = _rect.x, .y = _rect.y};
	const double tankRightX{_rect.Right()};
	const double tankBottomY{_rect.Bottom()};
	const FPoint tankCenter{.x = tankPos.x + tankHalf.x, .y = tankPos.y + tankHalf.y};

	const double bulletWidth{_calibre.size.x};
	const double bulletHeight{_calibre.size.y};
	const FPoint bulletHalf{.x = bulletWidth / 2.0, .y = bulletHeight / 2.0};
	ObjRectangle bulletRect{.x = -1, .y = -1, .w = bulletWidth, .h = bulletHeight};

	if (const Direction dir = _direction;
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

Uuid ShootingBeh::Shot(const std::optional<Uuid> uuid)
{
	const ObjRectangle rect = GetBulletStartRect();
	if (rect.x < 0.0 || rect.y < 0.0)
	{
		//Try shooting outside screen
		return {};
	}

	//TODO: refactor to network event ShotBullet{rect, bulletResetProperty, uuid}
	const BulletResetProperty bulletResetProperty{
			.rect = rect,
			.dir = _direction,
			.health = 1,
			.author = _author,
			.authorUuid = _uuid,
			.calibre = _calibre,
	};

	const std::shared_ptr<Bullet> bullet = _bulletPool->SpawnBullet(bulletResetProperty, uuid);

	Log::Detail("bullet spawned " + bullet->GetName() + " uuid " + UuidUtils::GetStringUuid(bullet->GetUuid()));

	_events->EmitEvent(AddToSpawnQueueEvent{.obj = bullet});

	return bullet->GetUuid();
}
