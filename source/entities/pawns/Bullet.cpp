#include "entities/pawns/Bullet.h"
#include "utils/Log.h"
#include "application/GameConfig.h"
#include "behavior/MoveLikeBulletBeh.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/StatisticsEvents.h"
#include "entities/obstacles/BushTile.h"
#include "entities/obstacles/IceTile.h"
#include "entities/obstacles/WaterTile.h"
#include "entities/pawns/BulletResetProperty.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/GameMode.h"
#include "enums/TextureType.h"
#include "interfaces/IMoveBeh.h"
#include "utils/UuidUtils.h"

Bullet::Bullet(PawnProperty pawnProperty, const GameConfig& gameConfig, const BulletCalibre& calibre)
	: Pawn{std::move(pawnProperty), gameConfig, kCollision}
	, _calibre{calibre}
{
	_moveBeh = std::make_unique<MoveLikeBulletBeh>(_rect, _uuid, _authorUuid, _gameConfig, _calibre);
}

Bullet::~Bullet()
{
	Log::Detail("bullet destroyed " + UuidUtils::GetStringUuid(_uuid));
}

void Bullet::Subscribe()
{
	Pawn::Subscribe();

	_subs.push_back(_events->AddListener(this, &Bullet::OnDraw));
}

void Bullet::OnDraw(const DrawEvent&) const { Draw(); }

void Bullet::OnDespawned(const DespawnedEvent& event)
{
	Pawn::OnDespawned(event);

	_events->EmitEvent(AnimationCreateBulletExplosionEvent{.rect = _rect});
}

void Bullet::Draw() const
{
	_events->EmitEvent(DrawObjEvent{.rect = _rect, .dir = _dir, .texture = TextureType::Bullet});
}

Uuid Bullet::GetUuid() const
{
	return _uuid;
}

void Bullet::Reset(const BulletResetProperty& resetProperty)
{
	SetRect(resetProperty.rect);
	SetHealth(resetProperty.health);
	SetDirection(resetProperty.dir);

	_author = resetProperty.author;
	_authorUuid = resetProperty.authorUuid;
	_faction = FactionOf(_author);
	_calibre = resetProperty.calibre;

	SetIsAlive(true);
}

void Bullet::TickUpdate(const double deltaTime)
{
	std::vector<std::shared_ptr<BaseObj>> outCollisions;
	const bool isMove = _moveBeh->Move(_dir, deltaTime, _allObjects, outCollisions);
	if (!isMove)
	{
		DealDamage(outCollisions);
		outCollisions.clear();
	}

	if (isMove && IsHost(_gameMode))
	{
		_events->EmitEvent(PosChangedEvent{.pos = GetPos(), .dir = _dir, .uuid = _uuid});
	}
}

unsigned int Bullet::GetDamage() const { return _calibre.damage; }

double Bullet::GetDamageRadius() const { return _calibre.damageRadius; }

void Bullet::EmitDamageStatistics(const Author author)
{
	_events->EmitEvent(StatisticsBulletHitEvent{.author = author});
}

unsigned int Bullet::GetTier() const { return _calibre.tier; }

void Bullet::DealDamage(const std::vector<std::shared_ptr<BaseObj>>& objectList)
{
	bool isBulletHitBullet{false};
	for (const auto& target: objectList)
	{
		if (target == nullptr)
		{
			continue;
		}

		auto* baseObj = target.get();
		if (dynamic_cast<WaterTile*>(baseObj) != nullptr
			|| dynamic_cast<BushTile*>(baseObj) != nullptr
			|| dynamic_cast<IceTile*>(baseObj) != nullptr)
		{
			continue;
		}

		if (target->GetIsDestructible() || _calibre.tier > 2u)
		{
			target->TakeDamage(_calibre.damage, _author);
			if (const auto* otherBullet = dynamic_cast<Bullet*>(baseObj))
			{
				isBulletHitBullet = true;
				//NOTE: in case another bullet hits this bullet, we take damage from another bullet and send statistics
				TakeDamage(otherBullet->GetDamage(), otherBullet->GetAuthor());
			}
		}
	}

	if (isBulletHitBullet == false)
	{
		//NOTE: call BaseObj::TakeDamage to skip statistic unnecessary record
		BaseObj::TakeDamage(_calibre.damage, _author);
	}

	_events->EmitEvent(AnimationCreateBulletExplosionEvent{.rect = _rect});
}
