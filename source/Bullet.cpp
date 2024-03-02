#include "../headers/Bullet.h"

Bullet::Bullet(const Point& pos, const int width, const int height, const int color, const int speed,
			   const Direction direction, const int health)
	: Pawn(pos, width, height, color, speed, health)
{
	SetDirection(direction);
	BaseObj::SetIsPassable(true);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);
}

Bullet::~Bullet() = default;

void Bullet::Move(Environment& env)
{
	const int speed = GetSpeed();

	if (const int direction = GetDirection(); direction == UP && GetY() - GetSpeed() >= 0) {
		const auto self = SDL_Rect{this->GetX(), this->GetY() - GetSpeed(), this->GetWidth(), this->GetHeight()};
		if (auto [isCanMove, pawn] = IsCanMove(&self, env); isCanMove) {
			MoveY(-speed);
		} else {
			DealDamage(pawn);
		}
	} else if (direction == DOWN && GetY() + GetSpeed() <= env.windowHeight) {
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
	} else if (direction == RIGHT && GetX() + GetSpeed() <= env.windowWidth) {
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

void Bullet::Draw(Environment& env) const { Pawn::Draw(env); }

void Bullet::KeyboardEvensHandlers(Environment& env, const Uint32 eventType, const SDL_Keycode key)
{
	Pawn::KeyboardEvensHandlers(env, eventType, key);
}

int Bullet::GetDamage() const { return _damage; }

void Bullet::Shot(Environment& env) {}

void Bullet::SetDamage(const int damage) { _damage = damage; }

std::tuple<bool, Pawn*> Bullet::IsCanMove(const SDL_Rect* self, const Environment& env) const
{
	for (auto* pawn: env.allPawns) {
		if (IsCollideWith(self, pawn)) {
			if (!pawn->GetIsPenetrable()) {
				return std::make_tuple(false, pawn);
			}

			return std::make_tuple(true, pawn);
		}
	}

	return std::make_tuple(true, nullptr);
}
