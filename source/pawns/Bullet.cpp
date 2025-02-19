#include "../../headers/pawns/Bullet.h"
#include "../../headers/EventSystem.h"
#include "../../headers/behavior/MoveLikeBulletBeh.h"

#include <string>

Bullet::Bullet(const ObjRectangle& rect, int damage, double aoeRadius, const int color, const int health,
               std::shared_ptr<int[]> windowBuffer, UPoint windowSize, const Direction direction, const float speed,
               std::vector<std::shared_ptr<BaseObj>>* allObjects, const std::shared_ptr<EventSystem>& events,
               std::string author, std::string fraction, int bulletId, const bool isNetworkControlled)
	: Pawn{rect,
	       color,
	       health,
	       std::move(windowBuffer),
	       std::move(windowSize),
	       direction,
	       speed,
	       allObjects,
	       events,
	       std::make_unique<MoveLikeBulletBeh>(this, allObjects, events)},
	  _author{std::move(author)},
	  _fraction{std::move(fraction)},
	  _bulletDamageAreaRadius{aoeRadius},
	  _damage{damage},
	  _isNetworkControlled{isNetworkControlled}
{
	BaseObj::SetIsPassable(true);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	_name = "Bullet" + std::to_string(bulletId);
	_id = bulletId;
	Subscribe();
}

Bullet::~Bullet()
{
	Unsubscribe();
}

void Bullet::Subscribe()
{
	if (_events == nullptr)
	{
		return;
	}

	_events->AddListener("Draw", _name, [this]() { this->Draw(); });

	if (_isNetworkControlled)
	{
		_events->AddListener<const FPoint, const Direction>(
				"ClientReceived_" + _name + "Pos", _name,
				[this](const FPoint newPos, const Direction direction)
				{
					this->SetDirection(direction);
					this->SetPos(newPos);
				});

		_events->AddListener<const int>("ClientReceived_" + _name + "Health", _name, [this](const int health)
		{
			this->SetHealth(health);
		});

		_events->AddListener("ClientReceived_" + _name + "Dispose", _name, [this]()
		{
			this->SetIsAlive(false);
		});

		return;
	}

	_events->AddListener<const float>("TickUpdate", _name, [this](const float deltaTime)
	{
		this->TickUpdate(deltaTime);
	});
}

void Bullet::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener("Draw", _name);

	if (_isNetworkControlled)
	{
		_events->RemoveListener<const FPoint, const Direction>("ClientReceived_" + _name + "Pos", _name);
		_events->RemoveListener<const int>("ClientReceived_" + _name + "Health", _name);
		_events->RemoveListener("ClientReceived_" + _name + "Dispose", _name);

		return;
	}

	_events->RemoveListener<const float>("TickUpdate", _name);
}

void Bullet::Disable() const
{
	Unsubscribe();
}

void Bullet::Enable()
{
	Subscribe();
}

void Bullet::Reset(const ObjRectangle& rect, const int damage, const double aoeRadius, const int color,
                   const float speed, const Direction direction, const int health, std::string author,
                   std::string fraction, const bool isNetworkControlled)
{
	SetShape(rect);
	SetColor(color);
	SetHealth(health);
	_moveBeh = std::make_unique<MoveLikeBulletBeh>(this, _allObjects, _events);
	SetDirection(direction);
	_author = std::move(author);
	_fraction = std::move(fraction);
	_damage = damage;
	_bulletDamageAreaRadius = aoeRadius;
	_speed = speed;
	SetIsAlive(true);
	_isNetworkControlled = isNetworkControlled;
	Enable();
}

void Bullet::TickUpdate(const float deltaTime)
{
	if (GetIsAlive())//TODO: maybe for all add check isAlive
	{
		_moveBeh->Move(deltaTime);

		// if (!_isNetworkControlled)
		// {
		_events->EmitEvent<const std::string&, const FPoint, const Direction>(
				"ServerSend_Pos", "Bullet" + std::to_string(GetId()), GetPos(), GetDirection());
		// }
	}
}

int Bullet::GetDamage() const
{
	return _damage;
}

double Bullet::GetBulletDamageAreaRadius() const
{
	return _bulletDamageAreaRadius;
}

std::string Bullet::GetName() const { return _name; }

std::string Bullet::GetAuthor() const
{
	return _author;
}

std::string Bullet::GetFraction() const
{
	return _fraction;
}

void Bullet::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>("BulletHit", author, fraction);
}

void Bullet::TakeDamage(const int damage)
{
	BaseObj::TakeDamage(damage);

	if (!_isNetworkControlled)
	{
		//TODO: move this to onHealthChange
		_events->EmitEvent<const std::string, const int>("ServerSend_Health", GetName(), GetHealth());
	}
}

int Bullet::GetId() const { return _id; }
