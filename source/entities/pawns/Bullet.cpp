#include "entities/pawns/Bullet.h"
#include "behavior/MoveLikeBulletBeh.h"
#include "components/EventSystem.h"
#include "entities/obstacles/GrassTile.h"
#include "entities/obstacles/IceTile.h"
#include "entities/obstacles/WaterTile.h"
#include "entities/pawns/BulletResetProperty.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/AnimationType.h"
#include "enums/GameMode.h"
#include "utils/UuidUtils.h"
// #include <iostream>

Bullet::Bullet(PawnProperty pawnProperty) : Bullet(std::move(pawnProperty), 0, 18.f, "") {}

Bullet::Bullet(PawnProperty pawnProperty, const int damage, const double aoeRadius, std::string author,
               const bool enableByDefault)
	: Pawn{std::move(pawnProperty)},
	  _author{std::move(author)},
	  _bulletDamageRadius{aoeRadius},
	  _damage{damage}
{
	BaseObj::SetIsPassable(true);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	_moveBeh = std::make_unique<MoveLikeBulletBeh>(_rect, _dir, _speed, _uuid, _bulletDamageRadius, _windowSize,
	                                               _bulletTargets, _allObjects);
	if (enableByDefault)
	{
		Bullet::Subscribe();
	}

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
	Bullet::Unsubscribe();
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
	_events->AddListener("ClientReceived_" + _name + "Dispose", _nameWithUuid, [this](const buuid& uuid)
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
	Pawn::Unsubscribe();

	_events->RemoveListener("Draw", _nameWithUuid);

	// std::cout << "[" << "Bullet::Unsubscribe" << "] "
	// 			<< "[" << (_gameMode == PlayAsHost ? "SERVER" : "CLIENT") << "] "
	// 			<< ", name=" << _name
	// 			<< ", name+UUID=" << _nameWithUuid
	// 			<< std::endl;
	if (_gameMode == GameMode::PlayAsClient)
	{
		UnsubscribeAsClient();
	}
}

void Bullet::UnsubscribeAsClient() const
{
	_events->RemoveListener("ClientReceived_" + _name + "Dispose", _nameWithUuid);
}

void Bullet::Draw() const { _events->EmitEvent("DrawObj", _rect, _dir, _name, _color); }

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
	SetColor(resetProperty.color);
	SetHealth(resetProperty.health);
	SetDirection(resetProperty.dir);

	//TODO: write reset for MoveLikeBulletBeh
	_moveBeh = std::make_unique<MoveLikeBulletBeh>(_rect, _dir, _speed, _uuid, _bulletDamageRadius, _windowSize,
	                                               _bulletTargets, _allObjects);
	_bulletTargets.clear();
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
		if (!Pawn::Move(deltaTime))
		{
			DealDamage(_bulletTargets);
			_bulletTargets.clear();
		}
	}
}

int Bullet::GetDamage() const { return _damage; }

double Bullet::GetBulletDamageRadius() const { return _bulletDamageRadius; }

std::string Bullet::GetAuthor() const { return _author; }

void Bullet::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent("Statistics_BulletHit", author, fraction);
}

void Bullet::TakeDamage(const int damage)
{
	Pawn::TakeDamage(damage);
}

int Bullet::GetTier() const { return _tier; }

void Bullet::DealDamage(const std::vector<std::shared_ptr<BaseObj>>& objectList)
{
	if (!objectList.empty())
	{
		for (const auto& target: objectList)
		{
			if (target && !dynamic_cast<WaterTile*>(target.get())
			    && !dynamic_cast<GrassTile*>(target.get())
			    && !dynamic_cast<IceTile*>(target.get())
			    && (target->GetIsDestructible() || _tier > 2))
			{
				target->TakeDamage(_damage);
				target->SendDamageStatistics(GetAuthor(), GetFraction());
				if (const auto* otherBullet = dynamic_cast<Bullet*>(target.get()))
				{
					SendDamageStatistics(otherBullet->GetAuthor(), otherBullet->GetFraction());
				}
			}
		}
	}

	TakeDamage(_damage);

	_events->EmitEvent("AnimationCreate", AnimationType::Bullet_Explosion, _rect, _name, _color);
}
