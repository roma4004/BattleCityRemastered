#include "../../headers/pawns/Bullet.h"
#include "../../headers/Point.h"
#include "../../headers/behavior/MoveLikeBulletBeh.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/GameMode.h"

#include <string>

Bullet::Bullet(const ObjRectangle& rect, int damage, double aoeRadius, const int color, const int health,
               std::shared_ptr<Window> window, const Direction dir, const float speed,
               std::vector<std::shared_ptr<BaseObj>>* allObjects, const std::shared_ptr<EventSystem>& events,
               std::string author, std::string fraction, const GameMode gameMode, int id, int tier)
	: Pawn{rect,
	       color,
	       health,
	       std::move(window),
	       dir,
	       speed,
	       allObjects,
	       events,
	       std::make_unique<MoveLikeBulletBeh>(this, allObjects, events),
	       id,
	       "Bullet" + std::to_string(id),
	       std::move(fraction),
	       tier,
		gameMode
	  },
	  _author{std::move(author)},
	  _bulletDamageRadius{aoeRadius},
	  _damage{damage}
{
	BaseObj::SetIsPassable(true);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	Subscribe();
}

Bullet::~Bullet()
{
	Unsubscribe();
}

void Bullet::Subscribe()
{
	if (_gameMode == PlayAsClient)
	{
		SubscribeAsClient();
	}
}

void Bullet::SubscribeAsClient()
{
	_events->AddListener("ClientReceived_" + _name + "Dispose", _name, [this]()
	{
		this->SetIsAlive(false);
	});
}

void Bullet::Unsubscribe() const
{
	if (_gameMode == PlayAsClient)
	{
		UnsubscribeAsClient();
	}
}

void Bullet::UnsubscribeAsClient() const
{
	_events->RemoveListener("ClientReceived_" + _name + "Dispose", _name);
}

void Bullet::Disable() const
{
	Pawn::Unsubscribe();
	Unsubscribe();
}

void Bullet::Enable()
{
	Pawn::Subscribe();
	Subscribe();
}

//TODO: call this from event subscription
void Bullet::Reset(const ObjRectangle& rect, const int damage, const double aoeRadius, const int color,
                   const float speed, const Direction dir, const int health, std::string author,
                   std::string fraction, const int tier)
{
	SetShape(rect);
	SetColor(color);
	SetHealth(health);
	_moveBeh = std::make_unique<MoveLikeBulletBeh>(this, _allObjects, _events);
	SetDirection(dir);
	_author = std::move(author);
	_fraction = std::move(fraction);
	_damage = damage;
	_bulletDamageRadius = aoeRadius;
	_speed = speed;
	SetIsAlive(true);
	Enable();
	_tier = tier;
}

void Bullet::TickUpdate(const float deltaTime)
{
	if (GetIsAlive())//TODO: maybe for all add check isAlive
	{
		_moveBeh->Move(deltaTime);

		if (_gameMode == PlayAsHost)
		{
			_events->EmitEvent<const std::string&, const FPoint, const Direction>(
					"ServerSend_Pos", "Bullet" + std::to_string(GetId()), GetPos(), GetDirection());
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
		_events->EmitEvent<const std::string&, const int>("ServerSend_Health", GetName(), GetHealth());
	}
}

int Bullet::GetTier() const { return _tier; }
