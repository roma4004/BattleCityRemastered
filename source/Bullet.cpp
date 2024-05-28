#include "../headers/Bullet.h"
#include "../headers/Circle.h"

Bullet::Bullet(const Rectangle& rect, const int color, const float speed, const Direction direction, const int health,
			   int* windowBuffer, const UPoint windowSize, std::vector<std::shared_ptr<BaseObj>>* allPawns,
			   std::shared_ptr<EventSystem> events)
	: Pawn{rect, color, speed, health, windowBuffer, windowSize, allPawns, std::move(events)}
{
	SetDirection(direction);
	BaseObj::SetIsPassable(true);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	// subscribe
	if (_events == nullptr)
	{
		return;
	}

	const auto name = "bullet " + std::to_string(reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));

	_events->AddListener<float>("TickUpdate", name, [this](const float deltaTime) { this->TickUpdate(deltaTime); });

	_events->AddListener("Draw", name, [this]() { this->Draw(); });
}

Bullet::~Bullet()
{
	// unsubscribe
	if (_events == nullptr)
	{
		return;
	}

	const auto name = "bullet " + std::to_string(reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));

	_events->RemoveListener<float>("TickUpdate", name);

	_events->RemoveListener("Draw", name);
}

void Bullet::Move(const float deltaTime)
{
	if (!GetIsAlive())
	{
		return;
	}

	const float speed = GetSpeed() * deltaTime;
	const float x = GetX();
	const float y = GetY();
	// const int width = GetWidth();
	// const int height = GetHeight();

	if (const int direction = GetDirection(); direction == UP && y - speed >= 0.0f)
	{
		if (const auto pawns = IsCanMove(deltaTime); pawns.empty())
		{
			MoveY(-speed);
		}
		else
		{
			DealDamage(pawns);
		}
	}
	else if (direction == DOWN && GetBottomSide() + speed <= static_cast<float>(_windowSize.y))
	{
		if (const auto pawns = IsCanMove(deltaTime); pawns.empty())
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
		if (const auto pawns = IsCanMove(deltaTime); pawns.empty())
		{
			MoveX(-speed);
		}
		else
		{
			DealDamage(pawns);
		}
	}
	else if (direction == RIGHT && GetRightSide() + speed <= static_cast<float>(_windowSize.x))
	{
		if (const auto pawns = IsCanMove(deltaTime); pawns.empty())
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

std::list<std::weak_ptr<BaseObj>> Bullet::IsCanMove(const float deltaTime)
{
	const Direction direction = GetDirection();
	float speedX = GetSpeed() * deltaTime;
	float speedY = GetSpeed() * deltaTime;

	// For some reason I can't make rect1 in if's Rider say I make unused object. So I made more crutches
	if (direction == UP)
	{
		//36 37 initialize in if
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
	const auto bulletNextPosRect =
			Rectangle{this->GetX() + speedX, this->GetY() + speedY, this->GetWidth(), this->GetHeight()};
	for (const std::shared_ptr<BaseObj>& pawn: *_allPawns)
	{
		if (this == pawn.get())
		{
			continue;
		}

		if (IsCollideWith(bulletNextPosRect, pawn->GetShape()))
		{
			if (!pawn->GetIsPenetrable())
			{
				//CheckAoE(_env, deltaTime, aoeList);
				CheckCircleAoE(FPoint{this->GetX() + speedX, this->GetY() + speedY}, aoeList);
				return aoeList;
			}
		}
	}

	return aoeList;
}

void Bullet::CheckCircleAoE(const FPoint blowCenter, std::list<std::weak_ptr<BaseObj>>& aoeList) const
{
	const Circle circle{blowCenter, 12};
	for (const std::shared_ptr<BaseObj>& pawn: *_allPawns)
	{
		if (this == pawn.get())
		{
			continue;
		}

		if (CheckIntersection(circle, pawn->GetShape()))
		{
			aoeList.emplace_back(std::weak_ptr(pawn));
		}
	}
}

void Bullet::CheckAoE(const float deltaTime, std::list<std::weak_ptr<BaseObj>>& aoeList) const
{
	const float x = GetX();
	const float y = GetY();
	const float width = GetWidth();
	const float height = GetHeight();
	const float speed = (GetSpeed() * deltaTime) * 2;

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
		for (const std::shared_ptr<BaseObj>& pawn: *_allPawns)
		{
			if (this == pawn.get())
			{
				continue;
			}

			if (IsCollideWith(target, pawn->GetShape()))
			{
				aoeList.emplace_back(std::weak_ptr(pawn));
			}
		}
	}
}
