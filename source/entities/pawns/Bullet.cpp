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
#include "interfaces/IMoveBeh.h"
#include "utils/UuidUtils.h"

Bullet::Bullet(PawnProperty pawnProperty, const GameConfig& gameConfig, const BulletCalibre& calibre,
			   std::string author,
			   const bool enableByDefault)
	: Pawn{std::move(pawnProperty), gameConfig, kCollision}
	, _author{std::move(author)}
	, _calibre{calibre}
{
	// NOTE: needed only for tests, TODO in test use tank shoot for bulletPool use instead of creating bullet
	_moveBeh = std::make_unique<MoveLikeBulletBeh>(_rect, _dir, _uuid, _authorUuid, _gameConfig, _calibre);

	if (enableByDefault)
	{
		Bullet::Subscribe();
	}

	_name = "Bullet";
}

Bullet::~Bullet()
{
	Log::Detail("bullet destroyed " + _nameWithUuid);
}

void Bullet::Subscribe()
{
	Pawn::Subscribe();

	_subs.push_back(_events->AddListener(this, &Bullet::OnDraw));

	if (IsClient(_gameMode))
	{
		SubscribeAsClient();
	}
}

void Bullet::OnDraw(const DrawEvent&) const { Draw(); }

void Bullet::SubscribeAsClient()
{
	_subs.push_back(_events->AddListener(Key(_uuid), this, &Bullet::OnDespawned));
	_subs.push_back(_events->AddListener(Key(_uuid), this, &Bullet::OnPosChanged));
}

void Bullet::OnDespawned(const DespawnedEvent& event)
{
	Pawn::OnDespawned(event);

	_events->EmitEvent(AnimationCreateBulletExplosionEvent{.rect = _rect, .name = _name});
}

void Bullet::Draw() const { _events->EmitEvent(DrawObjEvent{.rect = _rect, .dir = _dir, .name = _name}); }

Uuid Bullet::GetUuid() const
{
	return _uuid;
}

const std::string& Bullet::GetUuidStr() const
{
	return _uuidStr;
}

void Bullet::Enable()
{
	Subscribe();
}

void Bullet::Disable()
{
	Log::Detail("bullet disabled " + _nameWithUuid);

	Unsubscribe();
}

void Bullet::Reset(BulletResetProperty resetProperty)
{
	Disable();//TODO: remove and unsubscribe in bullet pool on return

	SetRect(resetProperty.rect);
	SetHealth(resetProperty.health);
	SetDirection(resetProperty.dir);

	if (auto* moveBeh = dynamic_cast<MoveLikeBulletBeh*>(_moveBeh.get()))
	{
		moveBeh->Reset(resetProperty.calibre);
	}
	else
	{
		_moveBeh = std::make_unique<MoveLikeBulletBeh>(_rect, _dir, _uuid, _authorUuid, _gameConfig,
													   resetProperty.calibre);
	}

	_author = std::move(resetProperty.author);
	_authorUuid = resetProperty.authorUuid;
	_faction = std::move(resetProperty.faction);
	_calibre = resetProperty.calibre;

	if (resetProperty.uuid != UuidUtils::GetNilUuid())
	{
		_uuid = resetProperty.uuid;
		_uuidStr = UuidUtils::GetStringUuid(_uuid);
	}
	_nameWithUuid = _name + _uuidStr;

	SetIsAlive(true);

	Enable();
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
		_events->EmitEvent(PosChangedEvent{.who = _name, .pos = GetPos(), .dir = _dir, .uuid = _uuid});
	}
}

unsigned int Bullet::GetDamage() const { return _calibre.damage; }

double Bullet::GetDamageRadius() const { return _calibre.damageRadius; }

std::string Bullet::GetAuthor() const { return _author; }

void Bullet::EmitDamageStatistics(const std::string& author, Faction faction)
{
	_events->EmitEvent(StatisticsBulletHitEvent{.author = author, .faction = faction});
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
			target->TakeDamage(_calibre.damage, GetAuthor(), GetFaction());
			if (const auto* otherBullet = dynamic_cast<Bullet*>(baseObj))
			{
				isBulletHitBullet = true;
				//NOTE: in case another bullet hits this bullet, we take damage from another bullet and send statistics
				TakeDamage(otherBullet->GetDamage(), otherBullet->GetAuthor(), otherBullet->GetFaction());
			}
		}
	}

	if (isBulletHitBullet == false)
	{
		//NOTE: call BaseObj::TakeDamage to skip statistic unnecessary record
		BaseObj::TakeDamage(_calibre.damage, GetAuthor(), GetFaction());
	}

	_events->EmitEvent(AnimationCreateBulletExplosionEvent{.rect = _rect, .name = _name});
}

void Bullet::OnPosChanged(const PosChangedEvent& event)
{
	SetDirection(event.dir);
	SetPos(event.pos);
}
