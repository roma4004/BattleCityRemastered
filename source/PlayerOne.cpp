#include "../headers/PlayerOne.h"

PlayerOne::PlayerOne(const Point& pos, const int width, const int height, const int color, const int speed, const int health,
					 Environment* env)
	: Pawn(pos, width, height, color, speed, health, env)
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	// subscribe
	if (_env == nullptr) {
		return;
	}

	const std::string listenerName = "PlayerOne";
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

PlayerOne::~PlayerOne()
{
	// unsubscribe
	if (_env == nullptr) {
		return;
	}

	const std::string listenerName = "PlayerOne";
	_env->Events.RemoveListenerFromEvent("Draw", listenerName);

	_env->Events.RemoveListenerFromEvent("MarkDestroy", listenerName);

	_env->Events.RemoveListenerFromEvent("TickUpdate", listenerName);

	if (const auto it = std::ranges::find(_env->AllPawns, dynamic_cast<Pawn*>(this)); it != _env->AllPawns.end()) {
		_env->AllPawns.erase(it);
	}
}

void PlayerOne::KeyboardEvensHandlers(Environment& env, const Uint32 eventType, const SDL_Keycode key)
{
	if (eventType == SDL_KEYDOWN) {
		if (key == SDLK_a) {
			KeyboardButtons.a = true;
			SetDirection(Direction::LEFT);
		} else if (key == SDLK_d) {
			KeyboardButtons.d = true;
			SetDirection(Direction::RIGHT);
		} else if (key == SDLK_s) {
			KeyboardButtons.s = true;
			SetDirection(Direction::DOWN);
		} else if (key == SDLK_w) {
			KeyboardButtons.w = true;
			SetDirection(Direction::UP);
		}
	} else if (eventType == SDL_KEYUP) {
		if (key == SDLK_a) {
			KeyboardButtons.a = false;
		} else if (key == SDLK_d) {
			KeyboardButtons.d = false;
		} else if (key == SDLK_s) {
			KeyboardButtons.s = false;
		} else if (key == SDLK_w) {
			KeyboardButtons.w = false;
		} else if (key == SDLK_SPACE) {
			KeyboardButtons.shot = true;
		}
	}
}