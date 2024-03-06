#include "../headers/Brick.h"
#include "../headers/Environment.h"

#include <string>

Brick::Brick(const Point& pos, const int width, const int height, const int color, const int speed, const int health,
			 Environment* env)
	: BaseObj(pos, width, height, color, speed, health, env)
{
	// subscribe
	if (_env == nullptr) {
		return;
	}

	const auto eventName = std::to_string(reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));

	_env->Events.AddListenerToEvent("TickUpdate", eventName, [env = _env, self = dynamic_cast<Pawn*>(this)]() {
		self->TickUpdate(env);
	});

	_env->Events.AddListenerToEvent("MarkDestroy", eventName, [env = _env, self = dynamic_cast<Pawn*>(this)]() {
		self->MarkDestroy(env);
	});

	_env->Events.AddListenerToEvent("Draw", eventName, [env = _env, self = dynamic_cast<Pawn*>(this)]() {
		self->Draw(env);
	});
}

Brick::~Brick()
{
	// unsubscribe
	if (_env == nullptr) {
		return;
	}

	const auto eventName = std::to_string(reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));
	_env->Events.RemoveListenerFromEvent("Draw", eventName);

	_env->Events.RemoveListenerFromEvent("MarkDestroy", eventName);

	_env->Events.RemoveListenerFromEvent("TickUpdate", eventName);

	if (const auto it = std::ranges::find(_env->AllPawns, dynamic_cast<Pawn*>(this));
		it != _env->AllPawns.cend()) {
		_env->AllPawns.erase(it);
	}
}

void Brick::Draw(const Environment* env) const
{
	for (int y = GetY(); y < GetY() + GetHeight(); ++y) {
		for (int x = GetX(); x < GetX() + GetWidth(); ++x) {
			env->SetPixel(x, y, GetColor());
		}
	}
}