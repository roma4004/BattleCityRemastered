#include "entities/pawns/Bullet.h"
#include "behavior/MoveLikeBulletBeh.h"
#include "components/EventSystem.h"
#include "entities/obstacles/BushTile.h"
#include "entities/obstacles/IceTile.h"
#include "entities/obstacles/WaterTile.h"
#include "entities/pawns/BulletResetProperty.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/GameMode.h"
#include "interfaces/IMoveBeh.h"
#include "utils/UuidUtils.h"
// #include <iostream>

Bullet::Bullet(PawnProperty pawnProperty, const BulletCalibre& calibre, std::string author, const bool enableByDefault)
	: Pawn{std::move(pawnProperty)}
	, _author{std::move(author)}
	, _calibre{calibre}
{
	BaseObj::SetIsPassable(true);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	// NOTE: needed only for tests, TODO in test use tank shoot instead of creating bullet
	_moveBeh = std::make_unique<MoveLikeBulletBeh>(_rect, _dir, _uuid, _windowSize, _calibre, _allObjects);

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
	// 			<< std::endl;
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
			"ClientReceived_" + _name + "Dispose", _nameWithUuid,
			[this](const buuid& uuid)
			{
				if (uuid != _uuid)
				{
					return;
				}

				this->SetIsAlive(false);
			});
	_events->AddListener(
			"ClientReceived_" + _name + "Pos",
			_nameWithUuid,
			[this](const FPoint newPos, const Direction dir, const buuid& uuid)
			{
				OnClientChangePos(newPos, dir, uuid);
			});
}

void Bullet::Unsubscribe() const
{
	Pawn::Unsubscribe();
	_events->RemoveAllListeners(_nameWithUuid);
}

void Bullet::Draw() const { _events->EmitEvent("DrawObj", _rect, _dir, _name); }

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
	// 			<< std::endl;

	Unsubscribe();
}

void Bullet::Reset(BulletResetProperty resetProperty)
{
	Disable();

	SetRect(resetProperty.rect);
	SetHealth(resetProperty.health);
	SetDirection(resetProperty.dir);

	//TODO: write reset for MoveLikeBulletBeh
	_moveBeh = std::make_unique<MoveLikeBulletBeh>(_rect, _dir, _uuid, _windowSize, resetProperty.calibre, _allObjects);
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
			_events->EmitEvent("ServerSend_Pos", _name, GetPos(), _dir, _uuid);
		}
	}
}

int Bullet::GetDamage() const { return _calibre.damage; }

double Bullet::GetDamageRadius() const { return _calibre.damageRadius; }

std::string Bullet::GetAuthor() const { return _author; }

void Bullet::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent("Statistics_BulletHit", author, fraction);
}

void Bullet::TakeDamage(const int damage)
{
	Pawn::TakeDamage(damage);
}

int Bullet::GetTier() const { return _calibre.tier; }

void Bullet::DealDamage(const std::vector<std::shared_ptr<BaseObj>>& objectList)
{
	for (const auto& target: objectList)
	{
		if (target && !dynamic_cast<WaterTile*>(target.get())
			&& !dynamic_cast<BushTile*>(target.get())
			&& !dynamic_cast<IceTile*>(target.get())
			&& (target->GetIsDestructible() || _calibre.tier > 2))
		{
			target->TakeDamage(_calibre.damage);
			target->SendDamageStatistics(GetAuthor(), GetFraction());//TODO: move send dmg stat to takeDamage
			if (const auto* otherBullet = dynamic_cast<Bullet*>(target.get()))
			{
				SendDamageStatistics(otherBullet->GetAuthor(), otherBullet->GetFraction());
			}
		}
	}

	TakeDamage(_calibre.damage);

	_events->EmitEvent("AnimationCreateBulletExplosion", _rect, _name);
}

void Bullet::OnClientChangePos(const FPoint newPos, const Direction dir, const buuid& uuid)
{
	if (uuid != _uuid)//TODO: check maybe never true
	{
		return;
	}

	SetDirection(dir);
	SetPos(newPos);
}
