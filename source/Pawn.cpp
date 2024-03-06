#include "../headers/Pawn.h"
#include "../headers/Bullet.h"

Pawn::Pawn(const Point& pos, const int width, const int height, const int color, const int speed, const int health,
		   Environment* env)
	: BaseObj(pos, width, height, color, speed, health, env) {}

Pawn::~Pawn() = default;

void Pawn::MarkDestroy(Environment* env) const
{
	if (!GetIsAlive()) {
		env->PawnsToDestroy.emplace_back(const_cast<Pawn*>(this));
	}
}

void Pawn::Draw(const Environment* env) const
{
	for (int y = GetY(); y < GetY() + GetHeight(); ++y) {
		for (int x = GetX(); x < GetX() + GetWidth(); ++x) {
			env->SetPixel(x, y, GetColor());
		}
	}
}

bool Pawn::IsCollideWith(const SDL_Rect* self, const Pawn* other) const
{
	if (this == other) {
		return false;
	}

	const auto rect2 = SDL_Rect{other->GetX(), other->GetY(), other->GetWidth(), other->GetHeight()};
	SDL_Rect rect3;

	return SDL_IntersectRect(self, &rect2, &rect3);
}

std::tuple<bool, Pawn*> Pawn::IsCanMove(const SDL_Rect* self, const Environment* env) const
{
	for (auto* pawn: env->AllPawns) {
		if (IsCollideWith(self, pawn)) {
			if (!pawn->GetIsPassable()) {
				return std::make_tuple(false, pawn);
			}

			return std::make_tuple(true, pawn);
		}
	}

	return std::make_tuple(true, nullptr);
}

void Pawn::TickUpdate(Environment* env)
{
	/*Pawn* isItABullet =*/
	Move(env);
	Shot(env);
	/*if (isItABullet != nullptr){
		DealingDamage(isItABullet);
	}*/
}

void Pawn::Shot(Environment* env)
{
	if (KeyboardButtons.shot) {
		const Direction direction = GetDirection();
		const Point tankHalf = {GetWidth() / 2, GetHeight() / 2};
		const int x = GetX();
		const int y = GetY();
		const Point tankCenter = {x + tankHalf.x, y + tankHalf.y};
		const int width = GetBulletWidth();
		const int height = GetBulletHeight();
		const Point bulletHalf = {width / 2, height / 2};
		constexpr int color = 0xffffff;
		const int speed = GetBulletSpeed();
		constexpr int health = 1;

		if (direction == Direction::UP && y - bulletHalf.x - width >= 0u) {
			const Point pos = {tankCenter.x - bulletHalf.x, tankCenter.y - tankHalf.y - height - bulletHalf.y};
			env->AllPawns.emplace_back(new Bullet{pos, width, height, color, speed, direction, health, env});
		} else if (direction == Direction::DOWN && y + bulletHalf.y + height <= env->WindowHeight) {
			const Point pos = {tankCenter.x - bulletHalf.x, tankCenter.y + tankHalf.y + height + bulletHalf.y};
			env->AllPawns.emplace_back(new Bullet{pos, width, height, color, speed, direction, health, env});
		} else if (direction == Direction::LEFT && x - bulletHalf.x - width >= 0u) {
			const Point pos = {tankCenter.x - tankHalf.x - width - bulletHalf.x, tankCenter.y - bulletHalf.y};
			env->AllPawns.emplace_back(new Bullet{pos, width, height, color, speed, direction, health, env});
		} else if (direction == Direction::RIGHT && x + GetWidth() + bulletHalf.x + width <= env->WindowWidth) {
			const Point pos = {tankCenter.x + tankHalf.x + width + bulletHalf.x, tankCenter.y - bulletHalf.y};
			env->AllPawns.emplace_back(new Bullet{pos, width, height, color, speed, direction, health, env});
		}

		KeyboardButtons.shot = false;
	}
}

void Pawn::Move(Environment* env)
{
	const int speed = GetSpeed();
	if (KeyboardButtons.a && GetX() + speed >= 0) {
		const auto self = SDL_Rect{this->GetX() - this->GetSpeed(), this->GetY(), this->GetWidth(), this->GetHeight()};
		if (auto [isCanMove, pawn] = IsCanMove(&self, env); isCanMove) {
			MoveX(-speed);
		}
	}

	if (KeyboardButtons.d && GetX() + speed + GetWidth() < env->WindowWidth) {
		const auto self = SDL_Rect{this->GetX() + this->GetSpeed(), this->GetY(), this->GetWidth(), this->GetHeight()};
		if (auto [isCanMove, pawn] = IsCanMove(&self, env); isCanMove) {
			MoveX(speed);
		}
	}

	if (KeyboardButtons.w && GetY() + speed >= 0) {
		const auto self = SDL_Rect{this->GetX(), this->GetY() - this->GetSpeed(), this->GetWidth(), this->GetHeight()};
		if (auto [isCanMove, pawn] = IsCanMove(&self, env); isCanMove) {
			MoveY(-speed);
		}
	}

	if (KeyboardButtons.s && GetY() + speed + GetHeight() < env->WindowHeight) {
		const auto self = SDL_Rect{this->GetX(), this->GetY() + this->GetSpeed(), this->GetWidth(), this->GetHeight()};
		if (auto [isCanMove, pawn] = IsCanMove(&self, env); isCanMove) {
			MoveY(speed);
		}
	}
}

void Pawn::KeyboardEvensHandlers(Environment& env, Uint32 eventType, SDL_Keycode key) {}

Direction Pawn::GetDirection() const
{
	return _direction;
}

void Pawn::SetDirection(const Direction direction)
{
	_direction = direction;
}

int Pawn::GetBulletWidth() const
{
	return _bulletWidth;
}

void Pawn::SetBulletWidth(const int bulletWidth)
{
	_bulletWidth = bulletWidth;
}

int Pawn::GetBulletHeight() const
{
	return _bulletHeight;
}

void Pawn::SetBulletHeight(const int bulletHeight)
{
	_bulletHeight = bulletHeight;
}

int Pawn::GetBulletSpeed() const
{
	return _bulletSpeed;
}

void Pawn::SetBulletSpeed(const int bulletSpeed)
{
	_bulletSpeed = bulletSpeed;
}