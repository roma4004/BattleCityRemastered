#include "../headers/Water.h"
#include "../headers/Environment.h"

#include <string>

Water::Water(const FPoint& pos, const float width, const float height, const int color, const float speed,
			 const int health, Environment* env)
	: BaseObj(pos, width, height, color, speed, health, env)
{
	// subscribe
	if (_env == nullptr)
	{
		return;
	}

	const auto eventName = std::to_string(reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));

	_env->events.AddListenerToEvent("TickUpdate", eventName,
									[self = dynamic_cast<Pawn*>(this)]() { self->TickUpdate(); });

	_env->events.AddListenerToEvent("Draw", eventName, [self = dynamic_cast<Pawn*>(this)]() { self->Draw(); });
}

Water::Water(const FPoint& pos, Environment* env) : BaseObj(pos, env->gridSize, env->gridSize, 0x1e90ff, 0, 15, env)
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(false);
	BaseObj::SetIsPenetrable(true);

	// subscribe
	if (_env == nullptr)
	{
		return;
	}

	const auto eventName = std::to_string(reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));

	_env->events.AddListenerToEvent("Draw", eventName, [self = dynamic_cast<BaseObj*>(this)]() { self->Draw(); });
}

Water::~Water()
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

void Water::Draw() const
{
	for (int y = static_cast<int>(GetY()); y < static_cast<int>(GetY()) + GetHeight(); ++y)
	{
		for (int x = static_cast<int>(GetX()); x < static_cast<int>(GetX()) + GetWidth(); ++x)
		{
			_env->SetPixel(x, y, GetColor());
		}
	}
}

void Water::TickUpdate() {}
