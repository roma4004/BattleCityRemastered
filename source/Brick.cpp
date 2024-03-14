#include "../headers/Brick.h"
#include "../headers/Environment.h"

#include <string>

Brick::Brick(const Point& pos, const int width, const int height, const int color, const float speed, const int health,
			 Environment* env)
	: BaseObj(pos, width, height, color, speed, health, env)
{
	// subscribe
	if (_env == nullptr) {
		return;
	}

	const auto eventName = std::to_string(reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));

	_env->events.AddListenerToEvent("TickUpdate", eventName, [env = _env, self = dynamic_cast<Pawn*>(this)]() {
		self->TickUpdate(env);
	});

	_env->events.AddListenerToEvent("MarkDestroy", eventName, [env = _env, self = dynamic_cast<Pawn*>(this)]() {
		self->MarkDestroy(env);
	});

	_env->events.AddListenerToEvent("Draw", eventName, [env = _env, self = dynamic_cast<Pawn*>(this)]() {
		self->Draw(env);
	});
}

Brick::Brick(const Point& pos, Environment* env) : BaseObj(pos, env->gridSize - 1, env->gridSize - 1, 0x924b00, 0, 15, env)
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);
	
	// subscribe
	if (_env == nullptr) {
		return;
	}

	const auto eventName = std::to_string(reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));
	
	_env->events.AddListenerToEvent("MarkDestroy", eventName, [env = _env, self = dynamic_cast<BaseObj*>(this)]() {
		self->MarkDestroy(env);
	});

	_env->events.AddListenerToEvent("Draw", eventName, [env = _env, self = dynamic_cast<BaseObj*>(this)]() {
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

	_env->events.RemoveListenerFromEvent("Draw", eventName);

	_env->events.RemoveListenerFromEvent("MarkDestroy", eventName);

	if (const auto it = std::ranges::find(_env->allPawns, dynamic_cast<BaseObj*>(this));
		it != _env->allPawns.cend()) {
		_env->allPawns.erase(it);
	}
}

void Brick::Draw(const Environment* env) const
{
	for (int y = static_cast<int>(GetY()); y < static_cast<int>(GetY()) + GetHeight(); ++y) {
		for (int x = static_cast<int>(GetX()); x < static_cast<int>(GetX()) + GetWidth(); ++x) {
			env->SetPixel(x, y, GetColor());
		}
	}
}

void Brick::MarkDestroy(Environment* env) const
{
	if (!GetIsAlive()) {
		auto self = dynamic_cast<BaseObj*>(const_cast<Brick*>(this));
		env->pawnsToDestroy.emplace_back(self);
	}
}

void Brick::TickUpdate(Environment* env) {}