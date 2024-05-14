#include "../headers/Bullet.h"

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
									[self = dynamic_cast<Pawn*>(this)]()
									{
										self->TickUpdate();
									});

	_env->events.AddListenerToEvent("Draw", listenerName, [self = dynamic_cast<Pawn*>(this)]()
	{
		self->Draw();
	});
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
		if (auto [isCanMove, pawn] = IsCanMove(this); isCanMove)
		{
			MoveY(-speed);
		}
		else
		{
			DealDamage(pawn);
		}
	}
	else if (direction == DOWN && y + speed <= static_cast<float>(_env->windowHeight))
	{
		if (auto [isCanMove, pawn] = IsCanMove(this); isCanMove)
		{
			MoveY(speed);
		}
		else
		{
			DealDamage(pawn);
		}
	}
	else if (direction == LEFT && x - speed >= 0.0f)
	{
		if (auto [isCanMove, pawn] = IsCanMove(this); isCanMove)
		{
			MoveX(-speed);
		}
		else
		{
			DealDamage(pawn);
		}
	}
	else if (direction == RIGHT && x + speed <= static_cast<float>(_env->windowWidth))
	{
		if (auto [isCanMove, pawn] = IsCanMove(this); isCanMove)
		{
			MoveX(speed);
		}
		else
		{
			DealDamage(pawn);
		}
	}
	else // Self-destroy when edge of windows is reached
	{
		SetIsAlive(false);
	}
}

void Bullet::DealDamage(const std::list<std::weak_ptr<BaseObj>>& objectList)
{
	const int damage = GetDamage();
	if (!objectList.empty())
	{
		for (const auto& target : objectList)
		{
			if (const auto targetLock = target.lock(); targetLock && targetLock->GetIsDestructible())
			{
				targetLock->TakeDamage(damage);
			}
		}
	}

	TakeDamage(damage);
}

void Bullet::Draw() const
{
	Pawn::Draw();
}

int Bullet::GetDamage() const
{
	return _damage;
}

void Bullet::Shot()
{
}

void Bullet::SetDamage(const int damage)
{
	_damage = damage;
}

std::tuple<bool, std::list<std::weak_ptr<BaseObj>>> Bullet::IsCanMove(const BaseObj* me)
{
	const Direction direction = GetDirection();
	float speedX = GetSpeed() * _env->deltaTime;
	float speedY = GetSpeed() * _env->deltaTime;

	// For some reason I can't make rect1 in if's Rider say i make unused object. So I made more crutches
	if (direction == Direction::UP)
	{ //36 37 initialize in  if
		speedY *= -1;
		speedX *= 0;
	}
	else if (direction == Direction::DOWN)
	{
		speedY *= 1;
		speedX *= 0;
	}
	else if (direction == Direction::LEFT)
	{
		speedX *= -1;
		speedY *= 0;
	}
	else if (direction == Direction::RIGHT)
	{
		speedX *= 1;
		speedY *= 0;
	}

	std::list<std::weak_ptr<BaseObj>> aoeList{};
	const auto rect1 = Rectangle{me->GetX() + speedX, me->GetY() + speedY, me->GetWidth(), me->GetHeight()};
	for (auto& pawn : _env->allPawns)
	{
		if (me == pawn.get())
		{
			continue;
		}

		const auto rect2 = Rectangle{pawn->GetX(), pawn->GetY(), pawn->GetWidth(), pawn->GetHeight()};
		if (IsCollideWith(rect1, rect2))
		{
			if (!pawn->GetIsPenetrable())
			{
				CheckAoE(me, _env, aoeList);
				return std::make_tuple(false, aoeList);
			}

			return std::make_tuple(true, aoeList);
		}
	}

	return std::make_tuple(true, aoeList);
}

void Bullet::CheckAoE(const BaseObj* me, const Environment* env, std::list<std::weak_ptr<BaseObj>>& aoeList) const
{
	const float x = me->GetX();
	const float y = me->GetY();
	const float width = me->GetWidth();
	const float height = me->GetHeight();
	const float speed = (GetSpeed() * env->deltaTime) * 2;
	for (const std::list<Rectangle> targetList{
			 // 123
			 // 4_6
			 // 789
			 // NOTE: where _ is this bullet
			 {x - speed, y - speed, width, height},
			 {x, y - speed, width, height},
			 {x + speed, y - speed, width, height},
			 {x - speed, y, width, height},
			 {x + speed, y, width, height},
			 {x - speed, y + speed, width, height},
			 {x, y + speed, width, height},
			 {x + speed, y + speed, width, height}
		 }; auto target : targetList)
	{
		for (const auto& pawn : env->allPawns)
		{
			if (me == pawn.get())
			{
				continue;
			}

			const auto rect2 = Rectangle{
				pawn->GetX(), pawn->GetY(), pawn->GetWidth(),
				pawn->GetHeight()
			};
			if (IsCollideWith(target, rect2))
			{
				aoeList.emplace_back(std::weak_ptr(pawn));
			}
		}
	}
}
