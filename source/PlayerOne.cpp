#include "../headers/PlayerOne.h"

PlayerOne::PlayerOne(const FPoint& pos, const float width, const float height, const int color, const float speed,
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
									[self = dynamic_cast<Pawn*>(this)]() { self->TickUpdate(); });

	_env->events.AddListenerToEvent("Draw", listenerName, [self = dynamic_cast<Pawn*>(this)]() { self->Draw(); });
}

PlayerOne::PlayerOne(const FPoint& pos, const int color, Environment* env)
	: PlayerOne(pos, env->tankSize, env->tankSize, color, env->tankSpeed, env->tankHealth, env)
{
}

PlayerOne::~PlayerOne()
{
	// unsubscribe
	if (_env == nullptr)
	{
		return;
	}


	if (!_env->isGameOver)
	{
		const std::string listenerName = "PlayerOne";
		
		_env->events.RemoveListenerFromEvent("TickUpdate", listenerName);

		_env->events.RemoveListenerFromEvent("Draw", listenerName);
	}
}

void PlayerOne::KeyboardEvensHandlers(const Uint32 eventType, const SDL_Keycode key)
{
	if (eventType == SDL_KEYDOWN)
	{
		if (key == SDLK_a)
		{
			keyboardButtons.a = true;
		}
		else if (key == SDLK_d)
		{
			keyboardButtons.d = true;
		}
		else if (key == SDLK_s)
		{
			keyboardButtons.s = true;
		}
		else if (key == SDLK_w)
		{
			keyboardButtons.w = true;
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
