#include "../headers/Bullet.h"
#include "../headers/EventSystem.h"

#include <string>

Bullet::Bullet(const Rectangle& rect, int damage, double aoeRadius, const int color, const float speed,
               const Direction direction, const int health, int* windowBuffer, const UPoint windowSize,
               std::vector<std::shared_ptr<BaseObj>>* allPawns, std::shared_ptr<EventSystem> events)
	: Pawn{rect,
	       color,
	       health,
	       windowBuffer,
	       windowSize,
	       allPawns,
	       std::move(events),
	       std::make_shared<MoveLikeBulletBeh>(direction, windowSize, speed, damage, aoeRadius, this, allPawns)}
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

	_events->AddListener<float>("TickUpdate", _name, [this](const float deltaTime) { this->TickUpdate(deltaTime); });

	_events->AddListener("Draw", _name, [this]() { this->Draw(); });
}

void Bullet::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener<float>("TickUpdate", _name);

	_events->RemoveListener("Draw", _name);
}

void Bullet::Reset(const Rectangle& rect, int damage, double aoeRadius, const int color, const float speed,
                   const Direction direction, const int health)
{
	SetShape(rect);
	SetColor(color);
	SetHealth(health);
	_moveBeh = std::make_shared<MoveLikeBulletBeh>(direction, _windowSize, speed, damage, aoeRadius, this, _allPawns);
	_moveBeh->SetDirection(direction);// TODO: move this to constructor
	Subscribe();
	SetIsAlive(true);
}

void Bullet::Move(const float deltaTime)
{
	if (!GetIsAlive())
	{
		return;
	}

	constexpr int sideBarWidth = 175;
	const float speed = _moveBeh->GetSpeed() * deltaTime;
	if (const int direction = _moveBeh->GetDirection(); direction == UP && GetY() - speed >= 0.0f)
	{
		_moveBeh->MoveUp(deltaTime);
	}
	else if (direction == DOWN && GetBottomSide() + speed <= static_cast<float>(_windowSize.y))
	{
		_moveBeh->MoveDown(deltaTime);
	}
	else if (direction == LEFT && GetX() - speed >= 0.0f)
	{
		_moveBeh->MoveLeft(deltaTime);
	}
	else if (direction == RIGHT && GetRightSide() + speed <= static_cast<float>(_windowSize.x) - sideBarWidth)
	{
		_moveBeh->MoveRight(deltaTime);
	}
	else// Self-destroy when edge of windows is reached
	{
		SetIsAlive(false);
	}
}
