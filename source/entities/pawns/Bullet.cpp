#include "entities/pawns/Bullet.h"
#include "application/GameConfig.h"
#include "behavior/MoveLikeBulletBeh.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/ObstacleAndBonusEvents.h"
#include "components/events/ReplicationEvents.h"
#include "entities/obstacles/BushTile.h"
#include "entities/obstacles/IceTile.h"
#include "entities/obstacles/WaterTile.h"
#include "entities/pawns/BulletResetProperty.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/GameMode.h"
#include "interfaces/IMoveBeh.h"
#include "utils/UuidUtils.h"
// #include <iostream>

Bullet::Bullet(PawnProperty pawnProperty, GameConfig& gameConfig, const BulletCalibre& calibre, std::string author,
			   const bool enableByDefault)
	: Pawn{std::move(pawnProperty), gameConfig}
	, _author{std::move(author)}
	, _calibre{calibre}
{
	BaseObj::SetIsPassable(true);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	// NOTE: needed only for tests, TODO in test use tank shoot instead of creating bullet
	_moveBeh = std::make_unique<MoveLikeBulletBeh>(_rect, _dir, _uuid, _gameConfig, _calibre, _allObjects);

	if (enableByDefault)
	{
		Bullet::Subscribe();
	}

	_name = "Bullet";
}

Bullet::~Bullet()
{
	// std::cout << "[" << "Bullet::~Bullet()" << "] "
	// 			<< "[" << (_gameMode == PlayAsHost ? "SERVER" : "CLIENT") << "] "
	// 			<< ", name=" << _name
	// 			<< ", name+UUID=" << _nameWithUuid
	// 			<< '\n';
	Unsubscribe();
}

void Bullet::Subscribe()
{
	Pawn::Subscribe();

	_events->AddListener("Draw", _nameWithUuid, [this]() { this->Draw(); });

	if (_gameMode == GameMode::PlayAsClient)
	{
		SubscribeAsClient();
	}
}

void Bullet::SubscribeAsClient()
{
	_events->AddListener(
			"ClientReceived_Dispose", _uuid, _nameWithUuid,
			[this]()
			{
				this->SetIsAlive(false);
			});
	_events->AddListener(
			"ClientReceived_Pos", _uuid, _nameWithUuid,
			[this](const ClientReceivedPosEvent& event)
			{
				OnClientChangePos(event.pos, event.dir);
			});
}

void Bullet::Unsubscribe() const
{
	Pawn::Unsubscribe();
	_events->RemoveAllListeners(_nameWithUuid);
}

void Bullet::Draw() const { _events->EmitEvent("DrawObj", DrawObjEvent{.rect = _rect, .dir = _dir, .name = _name}); }

using buuid = boost::uuids::uuid;

buuid Bullet::GetUuid() const
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

void Bullet::Disable() const
{
	// std::cout << "[" << "Bullet::Disable()" << "] "
	// 			<< "[" << (_gameMode == PlayAsHost ? "SERVER" : "CLIENT") << "] "
	// 			<< ", name=" << _name
	// 			<< ", name+UUID=" << _nameWithUuid
	// 			<< '\n';

	Unsubscribe();
}

void Bullet::Reset(BulletResetProperty resetProperty)
{
	Disable();

	SetRect(resetProperty.rect);
	SetHealth(resetProperty.health);
	SetDirection(resetProperty.dir);

	//TODO: write reset for MoveLikeBulletBeh
	_moveBeh = std::make_unique<MoveLikeBulletBeh>(_rect, _dir, _uuid, _gameConfig, resetProperty.calibre, _allObjects);
	_author = std::move(resetProperty.author);
	_fraction = std::move(resetProperty.fraction);
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
	if (GetIsAlive())//TODO: maybe for all add check isAlive
	{
		std::vector<std::shared_ptr<BaseObj>> outCollisions;
		const bool isMove = _moveBeh->Move(_dir, deltaTime, outCollisions);
		if (!isMove)
		{
			DealDamage(outCollisions);
			outCollisions.clear();
		}

		if (isMove && _gameMode == GameMode::PlayAsHost)// NOTE: replication position to the client
		{
			_events->EmitEvent("ServerSend_Pos",
							   ServerSendPosEvent{.who = _name, .pos = GetPos(), .dir = _dir, .uuid = _uuid});
		}
	}
}

unsigned int Bullet::GetDamage() const { return _calibre.damage; }

double Bullet::GetDamageRadius() const { return _calibre.damageRadius; }

std::string Bullet::GetAuthor() const { return _author; }

void Bullet::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent("Statistics_BulletHit", StatisticsAttributionEvent{.author = author, .fraction = fraction});
}

void Bullet::TakeDamage(const unsigned int damage, const std::string& damageAuthor, const std::string& damageFraction)
{
	Pawn::TakeDamage(damage, damageAuthor, damageFraction);
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
			target->TakeDamage(_calibre.damage, GetAuthor(), GetFraction());
			if (const auto* otherBullet = dynamic_cast<Bullet*>(baseObj))
			{
				isBulletHitBullet = true;
				//NOTE: in case another bullet hits this bullet, we take damage from another bullet and send statistics
				TakeDamage(otherBullet->GetDamage(), otherBullet->GetAuthor(), otherBullet->GetFraction());
			}
		}
	}

	if (isBulletHitBullet == false)
	{
		//NOTE: call BaseObj::TakeDamage to skip statistic unnecessary record
		BaseObj::TakeDamage(_calibre.damage, GetAuthor(), GetFraction());
	}

	_events->EmitEvent("AnimationCreateBulletExplosion", AnimationCreateExplosionEvent{.rect = _rect, .name = _name});
}

void Bullet::OnClientChangePos(const FPoint newPos, const Direction dir)
{
	SetDirection(dir);
	SetPos(newPos);
}
