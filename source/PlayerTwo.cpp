#include "../headers/PlayerTwo.h"

PlayerTwo::PlayerTwo(const Point& pos, const int width, const int height, const int color, const int speed, const int health, Environment* env)
	: Pawn(pos, width, height, color, speed, health, env)
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	// subscribe
	if (_env == nullptr) {
		return;
	}

	const std::string listenerName = "PlayerTwo";
	_env->Events.AddListenerToEvent("TickUpdate", listenerName, [env = _env, self = dynamic_cast<Pawn*>(this)]() {
		self->TickUpdate(env);
	});

	_env->Events.AddListenerToEvent("MarkDestroy", listenerName, [env = _env, self = dynamic_cast<Pawn*>(this)]() {
		self->MarkDestroy(env);
	});

	_env->Events.AddListenerToEvent("Draw", listenerName, [env = _env, self = dynamic_cast<Pawn*>(this)]() {
		self->Draw(env);
	});
}

PlayerTwo::~PlayerTwo()
{
	// unsubscribe
	if (_env == nullptr) {
		return;
	}

	const std::string listenerName = "PlayerTwo";
	_env->Events.RemoveListenerFromEvent("TickUpdate", listenerName);

	_env->Events.RemoveListenerFromEvent("MarkDestroy", listenerName);

	_env->Events.RemoveListenerFromEvent("Draw", listenerName);

	if (const auto it = std::ranges::find(_env->AllPawns, dynamic_cast<Pawn*>(this)); it != _env->AllPawns.end()) {
		_env->AllPawns.erase(it);
	}
}

void PlayerTwo::KeyboardEvensHandlers(Environment& env, const Uint32 eventType, const SDL_Keycode key)
{
	if (eventType == SDL_KEYDOWN) {
		if (key == SDLK_LEFT) {
			KeyboardButtons.a = true;
			SetDirection(Direction::LEFT);
		} else if (key == SDLK_RIGHT) {
			KeyboardButtons.d = true;
			SetDirection(Direction::RIGHT);
		} else if (key == SDLK_DOWN) {
			KeyboardButtons.s = true;
			SetDirection(Direction::DOWN);
		} else if (key == SDLK_UP) {
			KeyboardButtons.w = true;
			SetDirection(Direction::UP);
		}
	} else if (eventType == SDL_KEYUP) {
		if (key == SDLK_LEFT) {
			KeyboardButtons.a = false;
		} else if (key == SDLK_RIGHT) {
			KeyboardButtons.d = false;
		} else if (key == SDLK_DOWN) {
			KeyboardButtons.s = false;
		} else if (key == SDLK_UP) {
			KeyboardButtons.w = false;
		} else if (key == SDLK_RCTRL) {
			KeyboardButtons.shot = true;
		}
	}
}
