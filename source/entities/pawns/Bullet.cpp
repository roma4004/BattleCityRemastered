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
#include "components/WorldSnapshot.h"
#include "entities/pawns/BulletResetProperty.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/GameMode.h"
#include "enums/Terrain.h"
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

	if (_gameConfig.IsHost())
	{
		_subs.push_back(_events->AddListener(this, &Bullet::OnWorldSnapshotRequested));
	}
}

void Bullet::OnDraw(const DrawEvent&) const { Draw(); }

void Bullet::OnWorldSnapshotRequested(const WorldSnapshotRequestedEvent& event) const
{
	event.snapshot.bullets.push_back(BulletSnapshot{.author = _author, .uuid = _uuid, .rect = _rect, .dir = _dir});
}

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
	_rect = resetProperty.rect;
	SetHealth(resetProperty.health);
	_dir = resetProperty.dir;

	_author = resetProperty.author;
	_authorUuid = resetProperty.authorUuid;
	_faction = FactionOf(_author);
	_calibre = resetProperty.calibre;

	_isAlive = true;
}

void Bullet::TickUpdate(const double deltaTime)
{
	std::vector<std::shared_ptr<BaseObj>> outCollisions;
	const bool isMove{_moveBeh->Move(_dir, deltaTime, _allObjects, outCollisions)};
	if (!isMove)
	{
		DealDamage(outCollisions);
		outCollisions.clear();
	}

	if (isMove && _gameConfig.IsHost())
	{
		_events->EmitEvent(PosChangedEvent{.pos = GetPos(), .dir = _dir, .uuid = _uuid});
	}
}

unsigned int Bullet::GetDamage() const noexcept { return _calibre.damage; }

double Bullet::GetDamageRadius() const noexcept { return _calibre.damageRadius; }

double Bullet::GetFlightSpeed() const noexcept { return _calibre.speed; }

unsigned int Bullet::GetTier() const noexcept { return _calibre.tier; }

void Bullet::DealDamage(const std::vector<std::shared_ptr<BaseObj>>& objectList)
{
	bool isBulletHitBullet{};
	for (const auto& target: objectList)
	{
		if (target == nullptr)
		{
			continue;
		}

		auto* baseObj{target.get()};
		//NOTE: no tier reaches water or ice; a bush is not here because the tier check below burns it
		if (const Terrain terrain{baseObj->GetTerrain()}; terrain == Terrain::Water || terrain == Terrain::Ice)
		{
			continue;
		}

		if (target->GetIsDestructible() || _calibre.tier > 2u)
		{
			target->TakeDamage(_calibre.damage, _author);
		}

		const auto* otherBullet{dynamic_cast<Bullet*>(baseObj)};
		if (otherBullet == nullptr)
		{
			continue;
		}

		isBulletHitBullet = true;
		//NOTE: this one was shot down just as much as it shot the other down - a hit each
		_events->EmitEvent(StatisticsBulletHitEvent{.author = otherBullet->GetAuthor()});
		TakeDamage(otherBullet->GetDamage(), otherBullet->GetAuthor());
	}

	//NOTE: the counter is about bullets meeting bullets - a wall, a tank and the edge of the field are
	//counted by rows of their own, and were only ever recorded here because self-damage went out as a hit
	if (isBulletHitBullet)
	{
		_events->EmitEvent(StatisticsBulletHitEvent{.author = _author});
	}

	if (isBulletHitBullet == false)
	{
		//NOTE: burns itself out where it stopped; BaseObj's skips Pawn's HealthChangedEvent, and nobody
		//shot it down, so no hit goes out with it
		BaseObj::TakeDamage(_calibre.damage, _author);
	}

	_events->EmitEvent(AnimationCreateBulletExplosionEvent{.rect = _rect});
}
