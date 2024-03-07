#include "../headers/Pawn.h"
#include "../headers/Bullet.h"

Pawn::Pawn(const Point& pos, const int width, const int height, const int color, const float speed, const int health,
		   Environment* env)
	: BaseObj(pos, width, height, color, speed, health, env) {}

Pawn::~Pawn() = default;

void Pawn::MarkDestroy(Environment* env) const
{
	if (!GetIsAlive()) {
		env->pawnsToDestroy.emplace_back(const_cast<Pawn*>(this));
		if (const auto it = std::ranges::find(env->allPawns, const_cast<Pawn*>(this)); it != env->allPawns.end()) {
  			env->allPawns.erase(it);
		}
	}
}

void Pawn::Draw(const Environment* env) const
{
	for (int y = static_cast<int>(GetY()); y < static_cast<int>(GetY()) + GetHeight(); ++y) {
		for (int x = static_cast<int>(GetX()); x < static_cast<int>(GetX()) + GetWidth(); ++x) {
			env->SetPixel(x, y, GetColor());
		}
	}
}

bool Pawn::IsCollideWith(const SDL_Rect* self, const Pawn* other) const
{
	if (this == other) {
		return false;
	}

	const auto rect2 = SDL_Rect{ static_cast<int>(other->GetX()), static_cast<int>(other->GetY()), static_cast<int>(other->GetWidth()), static_cast<int>(other->GetHeight())};
	SDL_Rect rect3;

	return SDL_IntersectRect(self, &rect2, &rect3);
}

std::tuple<bool, Pawn*> Pawn::IsCanMove(const SDL_Rect* self, const Environment* env) const
{
	for (auto* pawn: env->allPawns) {
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
	if (keyboardButtons.shot) {
		const Direction direction = GetDirection();
		const Point tankHalf = {GetWidth() / 2, GetHeight() / 2};
		const int tankX = static_cast<int>(GetX());
		const int tankY = static_cast<int>(GetY());
		const Point tankCenter = {tankX + tankHalf.x, tankY + tankHalf.y};

		const int bulletWidth = GetBulletWidth();
		const int bulletHeight = GetBulletHeight();
		const Point bulletHalf = {bulletWidth / 2, bulletHeight / 2};
		constexpr int color = 0xffffff;
		const int speed = GetBulletSpeed();
		constexpr int health = 1;
		Point pos;

		if (direction == Direction::UP && tankY - bulletHalf.x - bulletWidth >= 0u) {
			pos = {tankCenter.x - bulletHalf.x, tankCenter.y - tankHalf.y - bulletHeight - bulletHalf.y};
		} else if (direction == Direction::DOWN && tankY + bulletHalf.y + bulletHeight <= env->windowHeight) {
			pos = {tankCenter.x - bulletHalf.x, tankCenter.y + tankHalf.y + bulletHeight + bulletHalf.y};
		} else if (direction == Direction::LEFT && tankX - bulletHalf.x - bulletWidth >= 0u) {
			pos = {tankCenter.x - tankHalf.x - bulletWidth - bulletHalf.x, tankCenter.y - bulletHalf.y};
		} else if (direction == Direction::RIGHT && tankX + GetWidth() + bulletHalf.x + bulletWidth <= env->windowWidth) {
			pos = {tankCenter.x + tankHalf.x + bulletWidth + bulletHalf.x, tankCenter.y - bulletHalf.y};
		} else {
			keyboardButtons.shot = false;
			return;
		}

		env->allPawns.emplace_back(new Bullet{pos, bulletWidth, bulletHeight, color, speed, direction, health, env});

		keyboardButtons.shot = false;
	}
}

void Pawn::Move(Environment* env)
{
	const float speed = GetSpeed() * env->deltaTime;
	const float x = GetX();
	const float y = GetY();
	const int width = GetWidth();
	const int height = GetHeight();
	if (keyboardButtons.a && GetX() + speed >= 0.f) {
		const auto self = SDL_Rect{static_cast<int>(x - speed), static_cast<int>(y), width, height};
		if (auto [isCanMove, pawn] = IsCanMove(&self, env); isCanMove) {
			MoveX(-speed);
		}
	}

	if (keyboardButtons.d && GetX() + speed + static_cast<float>(width) < static_cast<float>(env->windowWidth)) {
		const auto self = SDL_Rect{static_cast<int>(x + speed), static_cast<int>(y), width, height};
		if (auto [isCanMove, pawn] = IsCanMove(&self, env); isCanMove) {
			MoveX(speed);
		}
	}

	if (keyboardButtons.w && GetY() + speed >= 0.0f) {
		const auto self = SDL_Rect{static_cast<int>(x), static_cast<int>(y - speed), width, height};
		if (auto [isCanMove, pawn] = IsCanMove(&self, env); isCanMove) {
			MoveY(-speed);
		}
	}

	if (keyboardButtons.s && GetY() + speed + static_cast<float>(height) < static_cast<float>(env->windowHeight)) {
		const auto self = SDL_Rect{static_cast<int>(x), static_cast<int>(y + speed), width, height};
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