#include "../headers/PlayerTwo.h"

PlayerTwo::PlayerTwo(const Point& pos, const int width, const int height, const int color, const float speed, const int health,
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

	const std::string listenerName = "PlayerTwo";

	_env->events.AddListenerToEvent("TickUpdate", listenerName, [env = _env, self = dynamic_cast<Pawn*>(this)]() {
		self->TickUpdate(env);
	});

	_env->events.AddListenerToEvent("MarkDestroy", listenerName, [env = _env, self = dynamic_cast<Pawn*>(this)]() {
		self->MarkDestroy(env);
	});

	_env->events.AddListenerToEvent("Draw", listenerName, [env = _env, self = dynamic_cast<Pawn*>(this)]() {
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

	_env->events.RemoveListenerFromEvent("TickUpdate", listenerName);

	_env->events.RemoveListenerFromEvent("MarkDestroy", listenerName);

	_env->events.RemoveListenerFromEvent("Draw", listenerName);

	if (const auto it = std::ranges::find(_env->allPawns, dynamic_cast<Pawn*>(this)); it != _env->allPawns.end()) {
		_env->allPawns.erase(it);
	}
}

void PlayerTwo::KeyboardEvensHandlers(Environment& env, const Uint32 eventType, const SDL_Keycode key)
{
	if (eventType == SDL_KEYDOWN) {
		if (key == SDLK_LEFT) {
			keyboardButtons.a = true;
			SetDirection(Direction::LEFT);
		} else if (key == SDLK_RIGHT) {
			keyboardButtons.d = true;
			SetDirection(Direction::RIGHT);
		} else if (key == SDLK_DOWN) {
			keyboardButtons.s = true;
			SetDirection(Direction::DOWN);
		} else if (key == SDLK_UP) {
			keyboardButtons.w = true;
			SetDirection(Direction::UP);
		}
	} else if (eventType == SDL_KEYUP) {
		if (key == SDLK_LEFT) {
			keyboardButtons.a = false;
		} else if (key == SDLK_RIGHT) {
			keyboardButtons.d = false;
		} else if (key == SDLK_DOWN) {
			keyboardButtons.s = false;
		} else if (key == SDLK_UP) {
			keyboardButtons.w = false;
		} else if (key == SDLK_RCTRL) {
			keyboardButtons.shot = true;
		}
	}
}