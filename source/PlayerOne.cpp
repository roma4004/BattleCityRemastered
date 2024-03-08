#include "../headers/PlayerOne.h"

PlayerOne::PlayerOne(const Point& pos, const int width, const int height, const int color, const float speed,
					 const int health, Environment* env)
	: Pawn(pos, width, height, color, speed, health, env)
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	// subscribe
	if (_env == nullptr)
	{
		return;
	}

	const std::string listenerName = "PlayerOne";

	_env->events.AddListenerToEvent("TickUpdate", listenerName,
									[env = _env, self = dynamic_cast<Pawn*>(this)]() { self->TickUpdate(env); });

	_env->events.AddListenerToEvent("MarkDestroy", listenerName,
									[env = _env, self = dynamic_cast<Pawn*>(this)]() { self->MarkDestroy(env); });

	_env->events.AddListenerToEvent("Draw", listenerName,
									[env = _env, self = dynamic_cast<Pawn*>(this)]() { self->Draw(env); });
}

PlayerOne::~PlayerOne()
{
	// unsubscribe
	if (_env == nullptr)
	{
		return;
	}

	const std::string listenerName = "PlayerOne";

	_env->events.RemoveListenerFromEvent("Draw", listenerName);

	_env->events.RemoveListenerFromEvent("MarkDestroy", listenerName);

	_env->events.RemoveListenerFromEvent("TickUpdate", listenerName);

	if (const auto it = std::ranges::find(_env->allPawns, dynamic_cast<Pawn*>(this)); it != _env->allPawns.end())
	{
		_env->allPawns.erase(it);
	}
}

void PlayerOne::KeyboardEvensHandlers(Environment& env, const Uint32 eventType, const SDL_Keycode key)
{
	if (eventType == SDL_KEYDOWN)
	{
		if (key == SDLK_a)
		{
			keyboardButtons.a = true;
			SetDirection(Direction::LEFT);
		}
		else if (key == SDLK_d)
		{
			keyboardButtons.d = true;
			SetDirection(Direction::RIGHT);
		}
		else if (key == SDLK_s)
		{
			keyboardButtons.s = true;
			SetDirection(Direction::DOWN);
		}
		else if (key == SDLK_w)
		{
			keyboardButtons.w = true;
			SetDirection(Direction::UP);
		}
	}
	else if (eventType == SDL_KEYUP)
	{
		if (key == SDLK_a)
		{
			keyboardButtons.a = false;
		}
		else if (key == SDLK_d)
		{
			keyboardButtons.d = false;
		}
		else if (key == SDLK_s)
		{
			keyboardButtons.s = false;
		}
		else if (key == SDLK_w)
		{
			keyboardButtons.w = false;
		}
		else if (key == SDLK_SPACE)
		{
			keyboardButtons.shot = true;
		}
	}
}