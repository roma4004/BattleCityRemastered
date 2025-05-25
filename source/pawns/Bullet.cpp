#include "../../headers/pawns/Bullet.h"
#include "../../headers/Point.h"
#include "../../headers/behavior/MoveLikeBulletBeh.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/GameMode.h"
#include "../../headers/pawns/PawnProperty.h"

// #include <iostream>
#include <string>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

Bullet::Bullet(PawnProperty pawnProperty, const int damage, const double aoeRadius, std::string author,
               const boost::uuids::uuid uuid, const std::string& uuidStr)
	: Pawn{std::move(pawnProperty),
	       std::make_unique<MoveLikeBulletBeh>(this, pawnProperty.allObjects, pawnProperty.events)
	  },
	  _author{std::move(author)},
	  _bulletDamageRadius{aoeRadius},
	  _damage{damage}
{
	BaseObj::SetIsPassable(true);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	if (uuid == boost::uuids::nil_uuid() || uuidStr == "")
	{
		static boost::uuids::random_generator uuidGenerator;
		_uuid = uuidGenerator();
		_uuidStr = boost::uuids::to_string(_uuid);
	}
	else
	{
		_uuid = uuid;
		_uuidStr = uuidStr;
	}

	_name = "Bullet";

	Subscribe();
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

boost::uuids::uuid Bullet::GetUuid() const
{
	return _uuid;
}

const std::string& Bullet::GetUuidStr() const
{
	return _uuidStr;
}

void Bullet::SubscribeAsClient()
{
	_events->AddListener<const boost::uuids::uuid>(
			"ClientReceived_" + _name + "Dispose", _nameWithUuid,
			[this](const boost::uuids::uuid uuid)
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

void Bullet::Reset(const ObjRectangle& rect, const int damage, const double aoeRadius, const int color,
                   const float speed, const Direction dir, const int health, std::string author,
                   std::string fraction, const int tier, const boost::uuids::uuid uuid)
{
	Disable();

	SetRect(rect);
	SetColor(color);
	SetHealth(health);
	_moveBeh = std::make_unique<MoveLikeBulletBeh>(this, _allObjects, _events);
	SetDirection(dir);
	_author = std::move(author);
	_fraction = std::move(fraction);
	_damage = damage;
	_bulletDamageRadius = aoeRadius;
	_speed = speed;
	_tier = tier;

	if (uuid != boost::uuids::nil_uuid())
	{
		_uuid = uuid;
		_uuidStr = boost::uuids::to_string(_uuid);
	}
	_nameWithUuid = _name + _uuidStr;

	SetIsAlive(true);
	Enable();
}

void Bullet::TickUpdate(const float deltaTime)
{
	if (GetIsAlive())//TODO: maybe for all add check isAlive
	{
		if (_moveBeh->Move(deltaTime))
		{
			++_animationFrameId;
			if (_animationFrameId % 12 && ++_animationId > _animationIdLimit)
			{
				_animationId = 0;
				_animationFrameId = 0;
			}
		}

		if (_gameMode == PlayAsHost)
		{
			_events->EmitEvent<const std::string&, const FPoint, const Direction, const boost::uuids::uuid>(
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
		_events->EmitEvent<const std::string&, const int, const boost::uuids::uuid>(
				"ServerSend_Health", GetName(), GetHealth(), _uuid);
	}
}

int Bullet::GetTier() const { return _tier; }
