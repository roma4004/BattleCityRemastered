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
	       std::make_shared<MoveLikeBulletBeh>(windowSize, speed, damage, aoeRadius, this, allPawns)}
{
	_moveBeh->SetDirection(direction);
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
