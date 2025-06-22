#include "entities/pawns/Bullet.h"
#include "Point.h"
#include "behavior/MoveLikeBulletBeh.h"
#include "components/EventSystem.h"
#include "entities/pawns/BulletResetProperty.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/GameMode.h"
#include "utils/UuidUtils.h"
// #include <iostream>
#include <string>

Bullet::Bullet(PawnProperty pawnProperty) : Bullet(std::move(pawnProperty), 0, {18.f}, "") {}

Bullet::Bullet(PawnProperty pawnProperty, const int damage, const double aoeRadius, std::string author)
	: Pawn{pawnProperty, std::make_unique<MoveLikeBulletBeh>(this, pawnProperty.allObjects, pawnProperty.events)},
	  _author{std::move(author)},
	  _bulletDamageRadius{aoeRadius},
	  _damage{damage}
{
	BaseObj::SetIsPassable(true);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	if (_uuid == UuidUtils::GetNilUuid())
	{
		_uuid = UuidUtils::GetRandomUuid();
	}
	_uuidStr = UuidUtils::GetStringUuid(_uuid);

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
	if (_gameMode == PlayAsClient)
	{
		SubscribeAsClient();
	}
}

using buuid = boost::uuids::uuid;

buuid Bullet::GetUuid() const
{
	return _uuid;
}

const std::string& Bullet::GetUuidStr() const
{
	return _uuidStr;
}

void Bullet::SubscribeAsClient()
{
	_events->AddListener<const buuid&>("ClientReceived_" + _name + "Dispose", _nameWithUuid, [this](const buuid& uuid)
	{
		if (uuid != _uuid)
		{
			return;
		}

		this->SetIsAlive(false);
	});
}

void Bullet::Unsubscribe() const
{
	// std::cout << "[" << "Bullet::Unsubscribe" << "] "
	// 			<< "[" << (_gameMode == PlayAsHost ? "SERVER" : "CLIENT") << "] "
	// 			<< ", name=" << _name
	// 			<< ", name+UUID=" << _nameWithUuid
	// 			<< std::endl;
	if (_gameMode == PlayAsClient)
	{
		UnsubscribeAsClient();
	}
}

void Bullet::UnsubscribeAsClient() const
{
	_events->RemoveListener("ClientReceived_" + _name + "Dispose", _nameWithUuid);
}

void Bullet::Disable() const
{
	// std::cout << "[" << "Bullet::Disable()" << "] "
	// 			<< "[" << (_gameMode == PlayAsHost ? "SERVER" : "CLIENT") << "] "
	// 			<< ", name=" << _name
	// 			<< ", name+UUID=" << _nameWithUuid
	// 			<< std::endl;
	Pawn::Unsubscribe();
	Unsubscribe();
}

void Bullet::Enable()
{
	Pawn::Subscribe();
	Subscribe();
}

void Bullet::Reset(BulletResetProperty resetProperty)
{
	Disable();

	SetRect(resetProperty.rect);
	SetColor(resetProperty.color);
	SetHealth(resetProperty.health);
	SetDirection(resetProperty.dir);

	_moveBeh = std::make_unique<MoveLikeBulletBeh>(this, _allObjects, _events);
	_author = std::move(resetProperty.author);
	_fraction = std::move(resetProperty.fraction);
	_damage = resetProperty.damage;
	_bulletDamageRadius = resetProperty.aoeRadius;
	_speed = resetProperty.speed;
	_tier = resetProperty.tier;

	if (resetProperty.uuid != UuidUtils::GetNilUuid())
	{
		_uuid = resetProperty.uuid;
		_uuidStr = UuidUtils::GetStringUuid(_uuid);
	}
	_nameWithUuid = _name + _uuidStr;

	SetIsAlive(true);

	Enable();
}

void Bullet::TickUpdate(const float deltaTime)
{
	if (GetIsAlive())//TODO: maybe for all add check isAlive
	{
		//if (_moveBeh->Move(deltaTime)){}
		std::ignore = _moveBeh->Move(deltaTime);
		
		if (_gameMode == PlayAsHost)
		{
			_events->EmitEvent<const std::string&, const FPoint, const Direction, const buuid&>(
					"ServerSend_Pos", _name, GetPos(), GetDirection(), _uuid);
		}
	}
}

int Bullet::GetDamage() const { return _damage; }

double Bullet::GetBulletDamageRadius() const { return _bulletDamageRadius; }

std::string Bullet::GetAuthor() const { return _author; }

void Bullet::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>("Statistics_BulletHit", author, fraction);
}

void Bullet::TakeDamage(const int damage)
{
	BaseObj::TakeDamage(damage);

	if (_gameMode == PlayAsHost)
	{
		//TODO: move this to onHealthChange
		_events->EmitEvent<const std::string&, const int, const buuid&>(
				"ServerSend_Health", GetName(), GetHealth(), _uuid);
	}
}

int Bullet::GetTier() const { return _tier; }
