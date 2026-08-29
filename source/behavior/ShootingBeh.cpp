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
#include <functional>
#include <memory>
// #include <boost/uuid/uuid_io.hpp>

ShootingBeh::ShootingBeh(ObjRectangle& rect, Direction& dir, Uuid& uuid, std::string& name, Faction& faction,
						 const std::shared_ptr<BulletPool>& bulletPool,
						 BulletCalibre& calibre, const std::shared_ptr<EventSystem>& events,
						 const GameConfig& gameConfig)
	: _uuid{uuid}
	, _rect{rect}
	, _direction{dir}
	, _gameConfig{gameConfig}
	, _name{name}
	, _faction{faction}
	, _calibre{calibre}
	, _bulletPool{bulletPool}
	, _events{events} {}

ShootingBeh::~ShootingBeh() = default;

// inline float Distance(const FPoint a, const FPoint b)
// {
// 	return static_cast<float>(std::sqrt(std::pow(b.x - a.x, 2) + std::pow(b.y - a.y, 2)));
// }

double ShootingBeh::FindMinDistance(const std::vector<std::shared_ptr<BaseObj>>& objects,
									const std::function<double(const std::shared_ptr<BaseObj>&)>& sideDiff) const
{
	double minDist = static_cast<double>(_gameConfig.battlefieldSize.x * _gameConfig.battlefieldSize.y);
	// float nearestDist = 0.f;
	for (const auto& object: objects)
	{
		// auto getSide = [](const std::shared_ptr<BaseObj>& object) -> float { return object->GetX() + object->GetWidth();};
		const double distance = std::abs(sideDiff(object));
		// const float distance = abs(this->GetX() - object->GetX() + object->GetWidth());
		if (distance < minDist)//TODO: need minimal abs distance
		{
			minDist = distance;
		}
	}

	return minDist;

	// constexpr auto padding = 1.f;
	// float distance = this->GetX() - nearestX - padding;
	// if (distance < padding)
	// {
	// 	return 0.f;
	// }
	//
	// return distance;
}

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

Uuid ShootingBeh::Shot(const Uuid uuid)
{
	const ObjRectangle rect = GetBulletStartRect();
	if (rect.x < 0.0 || rect.y < 0.0)
	{
		//Try shooting outside screen
		return {};
	}

	//TODO: refactor to network event ShotBullet{rect, bulletResetProperty, uuid}
	auto bulletAsBase = _bulletPool->SpawnBullet();
	if (auto* bullet = dynamic_cast<Bullet*>(bulletAsBase.get()); bullet != nullptr)
	{
		BulletResetProperty bulletResetProperty = {
				.rect = rect,
				.dir = _direction,
				.health = 1,
				.author = _name,
				.faction = _faction,
				.uuid = uuid,
				.authorUuid = _uuid,
				.calibre = _calibre,
		};

		bullet->Reset(std::move(bulletResetProperty));

		Log::Detail("bullet reset " + bullet->GetName() + " uuid " + UuidUtils::GetStringUuid(bullet->GetUuid()));
	}

	if (bulletAsBase != nullptr)
	{
		_events->EmitEvent(AddToSpawnQueueEvent{bulletAsBase});
	}

	return bulletAsBase->GetUuid();
}

//TODO: create scheduled spawner manager to postpone or separate spawn to prevent change of subscription in event system
