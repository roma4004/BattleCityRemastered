#include "../headers/Brick.h"
#include "../headers/Environment.h"

#include <string>

Brick::Brick(const FPoint& pos, const float width, const float height, const int color, const float speed,
			 const int health, Environment* env)
	: BaseObj(pos, width, height, color, speed, health, env)
{
	// subscribe
	if (_env == nullptr)
	{
		return;
	}

	const auto eventName = std::to_string(reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));

	_env->events.AddListenerToEvent("TickUpdate", eventName, [this]() { this->TickUpdate(); });

	_env->events.AddListenerToEvent("Draw", eventName, [this]() { this->Draw(); });
}

Brick::Brick(const FPoint& pos, Environment* env)
	: BaseObj(pos, env->gridSize - 1, env->gridSize - 1, 0x924b00, 0, 15, env)
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	// subscribe
	if (_env == nullptr)
	{
		return;
	}

	const auto eventName = std::to_string(reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));

	_env->events.AddListenerToEvent("Draw", eventName, [this]() { this->Draw(); });
}

Brick::~Brick()
{
	// unsubscribe
	if (_env == nullptr)
	{
		return;
	}

	if (!_env->isGameOver)
	{
		const auto eventName = std::to_string(reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));

		_env->events.RemoveListenerFromEvent("Draw", eventName);
	}
}

void Brick::Draw() const
{
	int y = static_cast<int>(GetY());
	for (const int maxY = y + static_cast<int>(GetHeight()); y < maxY; ++y)
	{
		int x = static_cast<int>(GetX());
		for (const int maxX = x + static_cast<int>(GetWidth()); x < maxX; ++x)
		{
			_env->SetPixel(x, y, GetColor());
		}
	}
}

void Brick::TickUpdate() {}
