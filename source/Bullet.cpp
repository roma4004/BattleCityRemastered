#include "../headers/Bullet.h"

Bullet::Bullet(const Point& pos, const int width, const int height, const int color, const int speed,
			   const Direction direction, const int health, Environment* env)
	: Pawn(pos, width, height, color, speed, health, env)
{
	SetDirection(direction);
	BaseObj::SetIsPassable(true);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	// subscribe
	if (_env == nullptr) {
		return;
	}

	const auto listenerName = "bullet " + std::to_string(
									  reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));

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

Bullet::~Bullet()
{
	// unsubscribe
	if (_env == nullptr) {
		return;
	}

	if (const auto it = std::ranges::find(_env->AllPawns, dynamic_cast<Pawn*>(this));
		it != _env->AllPawns.end()) {
		_env->AllPawns.erase(it);
	}

	const auto listenerName = "bullet " + std::to_string(
									  reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));

	_env->Events.RemoveListenerFromEvent("Draw", listenerName);

	_env->Events.RemoveListenerFromEvent("MarkDestroy", listenerName);

	_env->Events.RemoveListenerFromEvent("TickUpdate", listenerName);

}

void Bullet::Move(Environment* env)
{
	const int speed = GetSpeed();

	if (const int direction = GetDirection(); direction == UP && GetY() - GetSpeed() >= 0) {
		const auto self = SDL_Rect{this->GetX(), this->GetY() - GetSpeed(), this->GetWidth(), this->GetHeight()};
		if (auto [isCanMove, pawn] = IsCanMove(&self, env); isCanMove) {
			MoveY(-speed);
		} else {
			DealDamage(pawn);
		}
	} else if (direction == DOWN && GetY() + GetSpeed() <= env->WindowHeight) {
		const auto self = SDL_Rect{this->GetX(), this->GetY() + GetSpeed(), this->GetWidth(), this->GetHeight()};
		if (auto [isCanMove, pawn] = IsCanMove(&self, env); isCanMove) {
			MoveY(speed);
		} else {
			DealDamage(pawn);
		}
	} else if (direction == LEFT && GetX() - GetSpeed() >= 0) {
		const auto self = SDL_Rect{this->GetX() - this->GetSpeed(), this->GetY(), this->GetWidth(), this->GetHeight()};
		if (auto [isCanMove, pawn] = IsCanMove(&self, env); isCanMove) {
			MoveX(-speed);
		} else {
			DealDamage(pawn);
		}
	} else if (direction == RIGHT && GetX() + GetSpeed() <= env->WindowWidth) {
		const auto self = SDL_Rect{this->GetX() + this->GetSpeed(), this->GetY(), this->GetWidth(), this->GetHeight()};
		if (auto [isCanMove, pawn] = IsCanMove(&self, env); isCanMove) {
			MoveX(speed);
		} else {
			DealDamage(pawn);
		}
	} else// Self-destroy when edge of windows is reached
	{
		SetIsAlive(false);
	}
}

void Bullet::DealDamage(Pawn* pawn)
{
	const int damage = GetDamage();
	pawn->TakeDamage(damage);
	TakeDamage(damage);
}

void Bullet::Draw(const Environment* env) const
{
	Pawn::Draw(env);
}

void Bullet::KeyboardEvensHandlers(Environment& env, const Uint32 eventType, const SDL_Keycode key)
{
	Pawn::KeyboardEvensHandlers(env, eventType, key);
}

int Bullet::GetDamage() const
{
	return _damage;
}

void Bullet::Shot(Environment* env) {}

void Bullet::SetDamage(const int damage)
{
	_damage = damage;
}

std::tuple<bool, Pawn*> Bullet::IsCanMove(const SDL_Rect* self, const Environment* env) const
{
	for (auto* pawn: env->AllPawns) {
		if (IsCollideWith(self, pawn)) {
			if (!pawn->GetIsPenetrable()) {
				return std::make_tuple(false, pawn);
			}

			return std::make_tuple(true, pawn);
		}
	}

	return std::make_tuple(true, nullptr);
}