#include "../headers/Bullet.h"
#include "../headers/Circle.h"

Bullet::Bullet(const FPoint& pos, const float width, const float height, const int color, const float speed,
			   const Direction direction, const int health, Environment* env)
	: Pawn(pos, width, height, color, speed, health, env)
{
	SetDirection(direction);
	BaseObj::SetIsPassable(true);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	// subscribe
	if (_env == nullptr)
	{
		return;
	}

	const auto listenerName =
			"bullet " + std::to_string(reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));

	_env->events.AddListenerToEvent("TickUpdate", listenerName,
									[self = dynamic_cast<Pawn*>(this)]() { self->TickUpdate(); });

	_env->events.AddListenerToEvent("Draw", listenerName, [self = dynamic_cast<Pawn*>(this)]() { self->Draw(); });
}

Bullet::~Bullet()
{
	// unsubscribe
	if (_env == nullptr)
	{
		return;
	}

	const auto listenerName =
			"bullet " + std::to_string(reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));

	_env->events.RemoveListenerFromEvent("Draw", listenerName);

	_env->events.RemoveListenerFromEvent("TickUpdate", listenerName);
}

void Bullet::Move()
{
	if (!GetIsAlive())
	{
		return;
	}

	const float speed = GetSpeed() * _env->deltaTime;
	const float x = GetX();
	const float y = GetY();
	// const int width = GetWidth();
	// const int height = GetHeight();

	if (const int direction = GetDirection(); direction == UP && y - speed >= 0.0f)
	{
		if (const auto pawns = IsCanMove(); pawns.empty())
		{
			MoveY(-speed);
		}
		else
		{
			DealDamage(pawns);
		}
	}
	else if (direction == DOWN && y + speed <= static_cast<float>(_env->windowHeight))
	{
		if (const auto pawns = IsCanMove(); pawns.empty())
		{
			MoveY(speed);
		}
		else
		{
			DealDamage(pawns);
		}
	}
	else if (direction == LEFT && x - speed >= 0.0f)
	{
		if (const auto pawns = IsCanMove(); pawns.empty())
		{
			MoveX(-speed);
		}
		else
		{
			DealDamage(pawns);
		}
	}
	else if (direction == RIGHT && x + speed <= static_cast<float>(_env->windowWidth))
	{
		if (const auto pawns = IsCanMove(); pawns.empty())
		{
			MoveX(speed);
		}
		else
		{
			DealDamage(pawns);
		}
	}
	else// Self-destroy when edge of windows is reached
	{
		SetIsAlive(false);
	}
}

void Bullet::DealDamage(const std::list<std::weak_ptr<BaseObj>>& objectList)
{
	const int damage = GetDamage();
	if (!objectList.empty())
	{
		for (const auto& target: objectList)
		{
			if (const auto targetLock = target.lock(); targetLock && targetLock->GetIsDestructible())
			{
				targetLock->TakeDamage(damage);
			}
		}
	}

	TakeDamage(damage);
}

void Bullet::Draw() const { Pawn::Draw(); }

int Bullet::GetDamage() const { return _damage; }

void Bullet::Shot() {}

void Bullet::SetDamage(const int damage) { _damage = damage; }

bool Bullet::CheckIntersection(const Circle& circle, const Rectangle& rect)
{
	const double deltaX = circle.center.x - std::max(rect.x, std::min(circle.center.x, rect.Right()));
	const double deltaY = circle.center.y - std::max(rect.y, std::min(circle.center.y, rect.Bottom()));

	return (deltaX * deltaX + deltaY * deltaY) < (circle.radius * circle.radius);
}

std::list<std::weak_ptr<BaseObj>> Bullet::IsCanMove()
{
	const Direction direction = GetDirection();
	float speedX = GetSpeed() * _env->deltaTime;
	float speedY = GetSpeed() * _env->deltaTime;

	// For some reason I can't make rect1 in if's Rider say i make unused object. So I made more crutches
	if (direction == UP)
	{
		//36 37 initialize in  if
		speedY *= -1;
		speedX *= 0;
	}
	else if (direction == DOWN)
	{
		speedY *= 1;
		speedX *= 0;
	}
	else if (direction == LEFT)
	{
		speedX *= -1;
		speedY *= 0;
	}
	else if (direction == RIGHT)
	{
		speedX *= 1;
		speedY *= 0;
	}

	std::list<std::weak_ptr<BaseObj>> aoeList{};
	const auto rect1 = Rectangle{this->GetX() + speedX, this->GetY() + speedY, this->GetWidth(), this->GetHeight()};
	for (auto& pawn: _env->allPawns)
	{
		if (this == pawn.get())
		{
			continue;
		}

		const auto rect2 = Rectangle{pawn->GetX(), pawn->GetY(), pawn->GetWidth(), pawn->GetHeight()};
		if (IsCollideWith(rect1, rect2))
		{
			if (!pawn->GetIsPenetrable())
			{
				//CheckAoE(_env, aoeList);
				CheckCircleAoE(_env, FPoint{this->GetX() + speedX, this->GetY() + speedY}, aoeList);
				return aoeList;
			}
		}
	}

	return aoeList;
}

void Bullet::CheckCircleAoE(const Environment* env, FPoint blowCenter, std::list<std::weak_ptr<BaseObj>>& aoeList) const
{
	Circle circle{blowCenter, 12};
	for (const auto& pawn: env->allPawns)
	{
		if (this == pawn.get())
		{
			continue;
		}

		const auto rect2 = Rectangle{pawn->GetX(), pawn->GetY(), pawn->GetWidth(), pawn->GetHeight()};

		if (CheckIntersection(circle, rect2))
		{
			aoeList.emplace_back(std::weak_ptr(pawn));
		}
	}
}

void Bullet::CheckAoE(const Environment* env, std::list<std::weak_ptr<BaseObj>>& aoeList) const
{
	const float x = GetX();
	const float y = GetY();
	const float width = GetWidth();
	const float height = GetHeight();
	const float speed = (GetSpeed() * env->deltaTime) * 2;

	for (const std::list<Rectangle> targetList{// NOTE: targets ordered in numpad positions
											   /*1*/ {x - speed, y - speed, width, height},
											   /*2*/ {x, y - speed, width, height},
											   /*3*/ {x + speed, y - speed, width, height},

											   /*4*/ {x - speed, y, width, height},
											   /*5*/ {x, y, width, height},// bullet position
											   /*6*/ {x + speed, y, width, height},

											   /*7*/ {x - speed, y + speed, width, height},
											   /*8*/ {x, y + speed, width, height},
											   /*9*/ {x + speed, y + speed, width, height}};
		 auto target: targetList)
	{
		for (const auto& pawn: env->allPawns)
		{
			if (this == pawn.get())
			{
				continue;
			}

			const auto rect2 = Rectangle{pawn->GetX(), pawn->GetY(), pawn->GetWidth(), pawn->GetHeight()};
			if (IsCollideWith(target, rect2))
			{
				aoeList.emplace_back(std::weak_ptr(pawn));
			}
		}
	}
}
