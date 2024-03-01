#include "../headers/Bullet.h"

Bullet::Bullet(const Point& pos, const int width, const int height,
			   const int color, const int speed, Direction direction,
			   int health)
	: Pawn(pos, width, height, color, speed, health) {
	SetDirection(direction);
}

Bullet::~Bullet() = default;

void Bullet::Move(Environment &env) {
	const int speed = GetSpeed();
	const int direction = GetDirection();

	if (direction == UP && GetY() - GetSpeed() >= 0) {
		const auto self = SDL_Rect{this->GetX(), this->GetY() - GetSpeed(),
								   this->GetWidth(), this->GetHeight()};
		auto [isCanMove, pawn] = IsCanMove(&self, env);
		if (isCanMove) {
			MoveY(-speed);
		}
		else {
			DealDamage(pawn);
		}
	} else if (direction == DOWN && GetY() + GetSpeed() <= env.windowHeight) {
		const auto self = SDL_Rect{this->GetX(), this->GetY() + GetSpeed(),
								   this->GetWidth(), this->GetHeight()};
		auto [isCanMove, pawn] = IsCanMove(&self, env);
		if (isCanMove) {
			MoveY(speed);
		}
		else {
			DealDamage(pawn);
		}
	} else if (direction == LEFT && GetX() - GetSpeed() >= 0) {
		const auto self = SDL_Rect{this->GetX() - this->GetSpeed(), this->GetY(),
								   this->GetWidth(), this->GetHeight()};
		auto [isCanMove, pawn] = IsCanMove(&self, env);
		if (isCanMove) {
			MoveX(-speed);
		}
		else {
			DealDamage(pawn);
		}
	} else if (direction == RIGHT && GetX() + GetSpeed() <= env.windowWidth) {
		const auto self = SDL_Rect{this->GetX() + this->GetSpeed(), this->GetY(),
								   this->GetWidth(), this->GetHeight()};
		auto [isCanMove, pawn] = IsCanMove(&self, env);
		if (isCanMove) {
			MoveX(speed);
		}
		else {
			DealDamage(pawn);
		}
	} else// Self-destroy when edge of windows is reached
	{
		SetIsAlive(false);
	}
}

void Bullet::DealDamage(Pawn *pawn) {
	const int damage = GetDamage();
	pawn->TakeDamage(damage);
	TakeDamage(damage);
}

void Bullet::Draw(Environment &env) const {
	Pawn::Draw(env);
}

void Bullet::KeyboardEvensHandlers(Environment &env, Uint32 eventType,
								   SDL_Keycode key) {
	Pawn::KeyboardEvensHandlers(env, eventType, key);
}

int Bullet::GetDamage() const {
	return _damage;
}

void Bullet::Shot(Environment &env) {}

void Bullet::SetDamage(const int damage) {
	_damage = damage;
}
