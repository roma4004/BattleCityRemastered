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

bool Pawn::IsCollideWith(const SDL_Rect* rect1, const SDL_Rect* rect2)
{
	SDL_Rect rect3;

	return SDL_IntersectRect(rect1, rect2, &rect3);
}

std::tuple<bool, std::list<BaseObj*>> Pawn::IsCanMove(const BaseObj* me, const Environment* env)
{
	const Direction direction = GetDirection();
	int speedX = static_cast<int>(GetSpeed() * env->deltaTime);
	int speedY = static_cast<int>(GetSpeed() * env->deltaTime);

	//const auto rect1 = SDL_Rect{static_cast<int>(me->GetX()) + speed, static_cast<int>(me->GetY()), me->GetWidth(), me->GetHeight()};
	if (direction == Direction::UP) { //36 37 initialize in  if
		speedY *= -1;
		speedX *= 0;
    } else if (direction == Direction::DOWN) {
    	speedY *= 1;
    	speedX *= 0;
    } else if (direction == Direction::LEFT) {
    	speedX *= -1;
    	speedY *= 0;
    } else if (direction == Direction::RIGHT) {
    	speedX *= 1;
    	speedY *= 0;
    }
	
	std::list<BaseObj*> obstacle{};
	const auto rect1 = SDL_Rect{ static_cast<int>(me->GetX()) + speedX, static_cast<int>(me->GetY()) + speedY, me->GetWidth(), me->GetHeight()};
	for (auto* pawn: env->allPawns) {
		if (me == pawn) {
			continue;
		}
		const auto rect2 = SDL_Rect{ static_cast<int>(pawn->GetX()), static_cast<int>(pawn->GetY()), pawn->GetWidth(), pawn->GetHeight()};
		if (IsCollideWith(&rect1, &rect2)) {
			if (!pawn->GetIsPassable()) {
				obstacle.emplace_back(pawn);
				return std::make_tuple(false, obstacle);
			}

			obstacle.emplace_back(pawn);
			return std::make_tuple(true, obstacle);
		}
	}

	return std::make_tuple(true, obstacle);
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
		const float speed = GetBulletSpeed();
		constexpr int health = 1;
		Point pos;

		if (direction == Direction::UP && tankY - bulletWidth >= 0u) {
			pos = {tankCenter.x - bulletHalf.x, tankCenter.y - bulletHalf.y - tankHalf.y};
		} else if (direction == Direction::DOWN && tankY + bulletHeight <= env->windowHeight) {
			pos = {tankCenter.x - bulletHalf.x, tankCenter.y + bulletHalf.y + tankHalf.y };
		} else if (direction == Direction::LEFT && tankX - bulletWidth >= 0u) {
			pos = {tankCenter.x - bulletHalf.x - tankHalf.x, tankCenter.y - bulletHalf.y};
		} else if (direction == Direction::RIGHT && tankX + GetWidth() + bulletHalf.x + bulletWidth <= env->windowWidth) {
			pos = {tankCenter.x + bulletHalf.x + tankHalf.x, tankCenter.y - bulletHalf.y};
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
	if (keyboardButtons.a && GetX()  >= 0.f + speed) {
		SetDirection(LEFT);
		if (auto [isCanMove, pawn] = IsCanMove(this, env); isCanMove) {
			MoveX(-speed);
		}
	} else if (keyboardButtons.d && GetX() + speed + static_cast<float>(width) < static_cast<float>(env->windowWidth)) {
		SetDirection(RIGHT);
		if (auto [isCanMove, pawn] = IsCanMove(this, env); isCanMove) {
			MoveX(speed);
		}
	} else if (keyboardButtons.w && GetY() >= 0.0f + speed) {
		SetDirection(UP);
		if (auto [isCanMove, pawn] = IsCanMove(this, env); isCanMove) {
			MoveY(-speed);
		}
	} else if (keyboardButtons.s && GetY() + speed + static_cast<float>(height) < static_cast<float>(env->windowHeight)) {
		SetDirection(DOWN);
		if (auto [isCanMove, pawn] = IsCanMove(this, env); isCanMove) {
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

float Pawn::GetBulletSpeed() const
{
	return _bulletSpeed;
}

void Pawn::SetBulletSpeed(const float bulletSpeed)
{
	_bulletSpeed = bulletSpeed;
}