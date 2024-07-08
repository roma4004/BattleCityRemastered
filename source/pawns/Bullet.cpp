#include "../../headers/pawns/Bullet.h"
#include "../../headers/EventSystem.h"

#include <string>

Bullet::Bullet(const Rectangle& rect, int damage, double aoeRadius, const int color, const int health,
               int* windowBuffer, const UPoint windowSize, const Direction direction, const float speed,
               std::vector<std::shared_ptr<BaseObj>>* allObjects, const std::shared_ptr<EventSystem>& events,
               std::string author, std::string fraction)
	: Pawn{rect,
	       color,
	       health,
	       windowBuffer,
	       windowSize,
	       direction,
	       speed,
	       allObjects,
	       events,
	       std::make_shared<MoveLikeBulletBeh>(this, allObjects, events)},
	  _author{std::move(author)}, _fraction{std::move(fraction)},
	  _damage{damage},
	  _bulletDamageAreaRadius{aoeRadius}
{
	BaseObj::SetIsPassable(true);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	_name = "bullet " + std::to_string(reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));
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

	_events->AddListener<const float>("TickUpdate", _name, [this](const float deltaTime)
	{
		this->TickUpdate(deltaTime);
	});

	_events->AddListener("Draw", _name, [this]() { this->Draw(); });
}

void Bullet::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener<const float>("TickUpdate", _name);

	_events->RemoveListener("Draw", _name);
}

void Bullet::Disable() const
{
	Unsubscribe();
}

void Bullet::Enable()
{
	Subscribe();
}

void Bullet::Reset(const Rectangle& rect, const int damage, const double aoeRadius, const int color, const float speed,
                   const Direction direction, const int health, std::string author, std::string fraction)
{
	SetShape(rect);
	SetColor(color);
	SetHealth(health);
	_moveBeh = std::make_shared<MoveLikeBulletBeh>(this, _allObjects, _events);
	SetDirection(direction);
	_author = std::move(author);
	_fraction = std::move(fraction);
	_damage = damage;
	_bulletDamageAreaRadius = aoeRadius;
	_speed = speed;
	SetIsAlive(true);
	Enable();
}

void Bullet::TickUpdate(const float deltaTime)
{
	if (GetIsAlive())//TODO: maybe for all add check isAlive
	{
		_moveBeh->Move(deltaTime);
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
