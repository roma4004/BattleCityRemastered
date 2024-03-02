#include "../headers/Pawn.h"
#include "../headers/Bullet.h"

Pawn::Pawn(const Point& pos, const int width, const int height, const int color, const int speed, const int health)
	: BaseObj(pos, width, height, color, speed, health) {}

Pawn::~Pawn() = default;

void Pawn::Draw(Environment& env) const {
	for (int y = GetY(); y < GetY() + GetHeight(); y++) {
		for (int x = GetX(); x < GetX() + GetWidth(); x++) { env.SetPixel(x, y, GetColor()); }
	}
}

bool Pawn::IsCollideWith(const SDL_Rect* self, const Pawn* other) const {
	if (this == other) { return false; }

	const auto rect2 = SDL_Rect{other->GetX(), other->GetY(), other->GetWidth(), other->GetHeight()};
	SDL_Rect rect3;

	return SDL_IntersectRect(self, &rect2, &rect3);
}

std::tuple<bool, Pawn*> Pawn::IsCanMove(const SDL_Rect* self, const Environment& env) const {
	for (auto* pawn: env.allPawns) {
		if (IsCollideWith(self, pawn)) {
			// TODO: check is IObsticle::isPassable, for bullet is true
			return std::make_tuple(false, pawn);
		}
	}

	return std::make_tuple(true, nullptr);
}

void Pawn::TickUpdate(Environment& env) {

	/*Pawn* isItABullet =*/Move(env);
	Shot(env);
	/*if (isItABullet != nullptr){
		DealingDamage(isItABullet);
	}*/
}

void Pawn::Shot(Environment& env) {
	if (keyboardButtons.shot) {
		Direction direction = GetDirection();
		const Point tankHalf = {static_cast<int>(GetWidth()) / 2, static_cast<int>(GetHeight()) / 2};
		int x = GetX();
		int y = GetY();
		const Point tankCenter = {x + tankHalf.x, y + tankHalf.y};
		int width = static_cast<int>(GetBulletWidth());
		int height = static_cast<int>(GetBulletHeight());
		const Point bulletHalf = {static_cast<int>(width) / 2, static_cast<int>(height) / 2};
		const int color = 0xffffff;
		const int speed = GetBulletSpeed();
		constexpr int health = 1;

		if (direction == Direction::UP && y - bulletHalf.x - width >= 0u) {
			const Point pos = {tankCenter.x - bulletHalf.x, tankCenter.y - tankHalf.y - height - bulletHalf.y};
			env.allPawns.emplace_back(new Bullet{pos, width, height, color, speed, direction, health});
		} else if (direction == Direction::DOWN && y + bulletHalf.y + height <= env.windowHeight) {
			const Point pos = {tankCenter.x - bulletHalf.x, tankCenter.y + tankHalf.y + height + bulletHalf.y};
			env.allPawns.emplace_back(new Bullet{pos, width, height, color, speed, direction, health});
		} else if (direction == Direction::LEFT && x - bulletHalf.x - width >= 0u) {
			const Point pos = {tankCenter.x - tankHalf.x - width - bulletHalf.x, tankCenter.y - bulletHalf.y};
			env.allPawns.emplace_back(new Bullet{pos, width, height, color, speed, direction, health});
		} else if (direction == Direction::RIGHT && x + GetWidth() + bulletHalf.x + width <= env.windowWidth) {
			const Point pos = {tankCenter.x + tankHalf.x + width + bulletHalf.x, tankCenter.y - bulletHalf.y};
			env.allPawns.emplace_back(new Bullet{pos, width, height, color, speed, direction, health});
		}

		keyboardButtons.shot = false;
	}
}

void Pawn::Move(Environment& env) {
	const int speed = GetSpeed();
	if (keyboardButtons.a && GetX() + speed >= 0) {
		const auto self = SDL_Rect{this->GetX() - this->GetSpeed(), this->GetY(), this->GetWidth(), this->GetHeight()};
		auto [isCanMove, pawn] = IsCanMove(&self, env);
		if (isCanMove) { MoveX(-speed); }
	}

	if (keyboardButtons.d && GetX() + speed + GetWidth() < env.windowWidth) {
		const auto self = SDL_Rect{this->GetX() + this->GetSpeed(), this->GetY(), this->GetWidth(), this->GetHeight()};
		auto [isCanMove, pawn] = IsCanMove(&self, env);
		if (isCanMove) { MoveX(speed); }
	}

	if (keyboardButtons.w && GetY() + speed >= 0) {
		const auto self = SDL_Rect{this->GetX(), this->GetY() - this->GetSpeed(), this->GetWidth(), this->GetHeight()};
		auto [isCanMove, pawn] = IsCanMove(&self, env);
		if (isCanMove) { MoveY(-speed); }
	}

	if (keyboardButtons.s && GetY() + speed + GetHeight() < env.windowHeight) {
		const auto self = SDL_Rect{this->GetX(), this->GetY() + this->GetSpeed(), this->GetWidth(), this->GetHeight()};
		auto [isCanMove, pawn] = IsCanMove(&self, env);
		if (isCanMove) { MoveY(speed); }
	}
}

void Pawn::KeyboardEvensHandlers(Environment& env, Uint32 eventType, SDL_Keycode key) {}

Direction Pawn::GetDirection() const { return _direction; }

void Pawn::SetDirection(const Direction direction) { _direction = direction; }

void Pawn::Destroy(Environment& env) const {
	auto it = std::ranges::find(env.allPawns, this);
	delete *it;
	env.allPawns.erase(it);
}

int Pawn::GetBulletWidth() const { return _bulletWidth; }

void Pawn::SetBulletWidth(int bulletWidth) { _bulletWidth = bulletWidth; }

int Pawn::GetBulletHeight() const { return _bulletHeight; }

void Pawn::SetBulletHeight(int bulletHeight) { _bulletHeight = bulletHeight; }

int Pawn::GetBulletSpeed() const { return _bulletSpeed; }

void Pawn::SetBulletSpeed(int bulletSpeed) { _bulletSpeed = bulletSpeed; }
