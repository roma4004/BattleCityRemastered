#include "../headers/Bullet.h"

Bullet::Bullet(const Point& pos, const int width, const int height, const int color, const float speed,
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

Bullet::~Bullet()
{
	// unsubscribe
	if (_env == nullptr) {
		return;
	}

	if (const auto it = std::ranges::find(_env->allPawns, dynamic_cast<Pawn*>(this));
		it != _env->allPawns.end()) {
		_env->allPawns.erase(it);
	}

	const auto listenerName = "bullet " + std::to_string(
									  reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));

	_env->events.RemoveListenerFromEvent("Draw", listenerName);

	_env->events.RemoveListenerFromEvent("MarkDestroy", listenerName);

	_env->events.RemoveListenerFromEvent("TickUpdate", listenerName);

}

void Bullet::Move(Environment* env)
{
	const float speed = GetSpeed() * env->deltaTime;
	const float x = GetX();
	const float y = GetY();
	const int width = GetWidth();
	const int height = GetHeight();
	
	
	if (const int direction = GetDirection(); direction == UP && y - speed >= 0.0f) {
		if (auto [isCanMove, pawn] = IsCanMove(this, env); isCanMove) {
			MoveY(-speed);
		} else {
			DealDamage(&pawn);
		}
	} else if (direction == DOWN && y + speed <= static_cast<float>(env->windowHeight)) {
		if (auto [isCanMove, pawn] = IsCanMove(this, env); isCanMove) {
			MoveY(speed);
		} else {
			DealDamage(&pawn);
		}
	} else if (direction == LEFT && x - speed >= 0.0f) {
		if (auto [isCanMove, pawn] = IsCanMove(this, env); isCanMove) {
			MoveX(-speed);
		} else {
			DealDamage(&pawn);
		}
	} else if (direction == RIGHT && x + speed <= static_cast<float>(env->windowWidth)) {
		if (auto [isCanMove, pawn] = IsCanMove(this, env); isCanMove) {
			MoveX(speed);
		} else {
			DealDamage(&pawn);
		}
	} else// Self-destroy when edge of windows is reached
	{
		SetIsAlive(false);
	}
}

void Bullet::DealDamage(const std::list<BaseObj*>* objectList)
{
	const int damage = GetDamage();
	if (!objectList->empty()) {
		for (auto* target : *objectList) {
			if (target->GetIsDestructible())
				target->TakeDamage(damage);
		}
	}
	
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

std::tuple<bool, std::list<BaseObj*>> Bullet::IsCanMove(const BaseObj* me, const Environment* env)
{
	const Direction direction = GetDirection();
	int speedX = static_cast<int>(GetSpeed() * env->deltaTime);
	int speedY = static_cast<int>(GetSpeed() * env->deltaTime);

	// For some reason I can't make rect1 in if's Rider say i make unused object. So I made more crutches
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

	std::list<BaseObj*> aoeList{};
	const auto rect1 = SDL_Rect{static_cast<int>(me->GetX()) + speedX, static_cast<int>(me->GetY()) + speedY, me->GetWidth(), me->GetHeight()};
	for (auto* pawn: env->allPawns) {
		if ( me == pawn) {
			continue;
		}
		
		const auto rect2 = SDL_Rect{ static_cast<int>(pawn->GetX()), static_cast<int>(pawn->GetY()), pawn->GetWidth(), pawn->GetHeight()};
		if (IsCollideWith(&rect1, &rect2)) {
			if (!pawn->GetIsPenetrable()) {
				CheckAoE(me, env, &aoeList);
				return std::make_tuple(false, aoeList);
			}

			return std::make_tuple(true, aoeList);
		}
	}

	return std::make_tuple(true, aoeList);
}

void Bullet::CheckAoE(const BaseObj* me, const Environment* env, std::list<BaseObj*>* aoeList) const
{
	// 123
	// 4_6
	// 789
	//const int meX = static_cast<int>(me->GetX());
	//const int meY = static_cast<int>(me->GetY());
	const float speed = (GetSpeed() * env->deltaTime) * 2;
	std::list<SDL_Rect>targetList;
	
	const auto targetOne = SDL_Rect{static_cast<int>(me->GetX() - speed), static_cast<int>(me->GetY() - speed), me->GetWidth(), me->GetHeight()};
	targetList.emplace_back(targetOne);
	const auto targetTwo = SDL_Rect{static_cast<int>(me->GetX()), static_cast<int>(me->GetY() - speed), me->GetWidth(), me->GetHeight()};
	targetList.emplace_back(targetTwo);
	const auto targetThree = SDL_Rect{static_cast<int>(me->GetX() + speed), static_cast<int>(me->GetY() - speed), me->GetWidth(), me->GetHeight()};
	targetList.emplace_back(targetThree);
	const auto targetFour = SDL_Rect{static_cast<int>(me->GetX() - speed), static_cast<int>(me->GetY()), me->GetWidth(), me->GetHeight()};
	targetList.emplace_back(targetFour);
	const auto targetSix = SDL_Rect{static_cast<int>(me->GetX() + speed), static_cast<int>(me->GetY()), me->GetWidth(), me->GetHeight()};
	targetList.emplace_back(targetSix);
	const auto targetSeven = SDL_Rect{static_cast<int>(me->GetX() - speed), static_cast<int>(me->GetY() + speed), me->GetWidth(), me->GetHeight()};
	targetList.emplace_back(targetSeven);
	const auto targetEight = SDL_Rect{static_cast<int>(me->GetX()), static_cast<int>(me->GetY() + speed), me->GetWidth(), me->GetHeight()};
	targetList.emplace_back(targetEight);
	const auto targetNine = SDL_Rect{static_cast<int>(me->GetX() + speed), static_cast<int>(me->GetY()+ speed), me->GetWidth(), me->GetHeight()};
	targetList.emplace_back(targetNine);

	for (auto target : targetList) {
		for (auto* pawn: env->allPawns) {
			if ( me == pawn) {
				continue;
			}
			const auto rect2 = SDL_Rect{ static_cast<int>(pawn->GetX()), static_cast<int>(pawn->GetY()), pawn->GetWidth(), pawn->GetHeight()};
			if (IsCollideWith(&target, &rect2)) {
				aoeList->emplace_back(pawn);
			}
		}
	}
}
