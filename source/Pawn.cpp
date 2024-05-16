#include <cmath>

#include "../headers/Bullet.h"
#include "../headers/Pawn.h"

Pawn::Pawn(const FPoint& pos, const float width, const float height, const int color, const float speed,
		   const int health, Environment* env)
	: BaseObj(pos, width, height, color, speed, health, env)
{
}

Pawn::~Pawn() = default;

void Pawn::Draw() const
{
	for (int y = static_cast<int>(GetY()); y < static_cast<int>(GetY() + GetHeight()); ++y)
	{
		for (int x = static_cast<int>(GetX()); x < static_cast<int>(GetX() + GetWidth()); ++x)
		{
			_env->SetPixel(x, y, GetColor());
		}
	}
}

bool Pawn::IsCollideWith(const Rectangle& r1, const Rectangle& r2)
{

	// Check if one rectangle is to the right of the other
	if (r1.x > r2.x + r2.w || r2.x > r1.x + r1.w)
	{
		return false;
	}

	// Check if one rectangle is above the other
	if (r1.y > r2.y + r2.h || r2.y > r1.y + r1.h)
	{
		return false;
	}

	// If neither of the above conditions are met, the rectangles overlap
	return true;
}

std::list<std::weak_ptr<BaseObj>> Pawn::IsCanMove()
{
	const Direction direction = GetDirection();
	float speedX = GetSpeed() * _env->deltaTime;
	float speedY = GetSpeed() * _env->deltaTime;

	//const auto rect1 = SDL_Rect{static_cast<int>(me->GetX()) + speed, static_cast<int>(me->GetY()), me->GetWidth(), me->GetHeight()};
	if (direction == UP)
	{
		//36 37 initialize in  if
		speedY *= -1;
		speedX *= 0;
	}
	else if (direction == DOWN)
	{
		speedY *= 1;
		speedX *= 0;
	}
	else if (direction == LEFT)
	{
		speedX *= -1;
		speedY *= 0;
	}
	else if (direction == RIGHT)
	{
		speedX *= 1;
		speedY *= 0;
	}

	std::list<std::weak_ptr<BaseObj>> obstacles{};
	const auto rect1 = Rectangle{this->GetX() + speedX, this->GetY() + speedY, this->GetWidth(), this->GetHeight()};
	for (auto& pawn: _env->allPawns)
	{
		if (this == pawn.get())
		{
			continue;
		}

		if (IsCollideWith(rect1, pawn->GetShape()))
		{
			if (!pawn->GetIsPassable())
			{
				//TODO: need fix, we broke collision detecting
				obstacles.emplace_back(std::weak_ptr(pawn));
				// return std::make_tuple(false, obstacle);
			}
			//else
			//{
			//	obstacle.emplace_back(std::weak_ptr(pawn));
			//	isHaveCollision = true;
			//	return std::make_tuple(true, obstacle);
			//}
		}
	}

	return obstacles;
}

void Pawn::TickUpdate()
{
	Move();
	Shot();
}

void Pawn::Shot()
{
	if (keyboardButtons.shot)
	{
		const Direction direction = GetDirection();
		const FPoint tankHalf = {GetWidth() / 2.f, GetHeight() / 2.f};
		const float tankX = GetX();
		const float tankY = GetY();
		const FPoint tankCenter = {tankX + tankHalf.x, tankY + tankHalf.y};

		const float bulletWidth = GetBulletWidth();
		const float bulletHeight = GetBulletHeight();
		const FPoint bulletHalf = {bulletWidth / 2.f, bulletHeight / 2.f};
		constexpr int color = 0xffffff;
		const float speed = GetBulletSpeed();
		constexpr int health = 1;
		FPoint pos;

		if (direction == UP && tankY - bulletWidth >= 0.f) //TODO: rewrite check with zero to use epsilon
		{
			pos = {tankCenter.x - bulletHalf.x, tankCenter.y - bulletHalf.y - tankHalf.y};
		}
		else if (direction == DOWN && tankY + bulletHeight <= static_cast<float>(_env->windowHeight))
		{
			pos = {tankCenter.x - bulletHalf.x, tankCenter.y + bulletHalf.y + tankHalf.y};
		}
		else if (direction == LEFT && tankX - bulletWidth >= 0.f) //TODO: rewrite check with zero to use epsilon
		{
			pos = {tankCenter.x - bulletHalf.x - tankHalf.x, tankCenter.y - bulletHalf.y};
		}
		else if (direction == RIGHT && tankX + GetWidth() + bulletHalf.x + bulletWidth <= static_cast<float>(_env->windowWidth))
		{
			pos = {tankCenter.x + bulletHalf.x + tankHalf.x, tankCenter.y - bulletHalf.y};
		}
		else
		{
			keyboardButtons.shot = false;
			return;
		}

		_env->allPawns.emplace_back(
				std::make_shared<Bullet>(pos, bulletWidth, bulletHeight, color, speed, direction, health, _env));

		keyboardButtons.shot = false;
	}
}

inline float Distance(const FPoint a, const FPoint b)
{
	return static_cast<float>(std::sqrt(std::pow(b.x - a.x, 2) + std::pow(b.y - a.y, 2)));
}


// double Distance(const SDL_FRect& a, const SDL_FRect& b) {
//     if (a.x + a.w < b.x // 'a' is left of 'b'
//         || b.x + b.w < a.x // 'a' is right of 'b'
//         || a.y < b.y + b.h // 'a' is above 'b'
//         || b.y < a.y + a.h) { // 'a' is below 'b'
// 		const double dx = std::max({a.x - b.x + b.w, b.x - a.x + a.w, 0.0f});
// 		const double dy = std::max(    {a.y - b.y + b.h, b.y - a.y + a.h, 0.0f});
//         return std::sqrt(dx*dx + dy*dy);
//     }
//     else {
//         // The rectangles intersect, so the distance is zero.
//         return 0.0;
//     }
// }
#include <functional>
#include <memory>

float Pawn::FindNearestDistance(const std::list<std::weak_ptr<BaseObj>>& pawns,
								const std::function<float(const std::shared_ptr<BaseObj>&)>& getNearestSide) const
{
	float nearestDist = _env->windowWidth * _env->windowHeight;
	// float nearestDist = 0;
	for (const auto& pawn: pawns)
	{
		const std::shared_ptr<BaseObj> pawnLck = pawn.lock();
		// auto getSide = [](const std::shared_ptr<BaseObj>& pawnLck) -> float { return pawnLck->GetX() + pawnLck->GetWidth();};
		const float distance = std::abs(getNearestSide(pawnLck));
		// const float distance = abs(this->GetX() - pawnLck->GetX() + pawnLck->GetWidth());
		if (distance < nearestDist)//TODO: need minimal abs distance
		{
			nearestDist = distance;
		}
	}

	return nearestDist;

	// constexpr auto padding = 1.f;
	// float distance = this->GetX() - nearestX - padding;
	// if (distance < padding)
	// {
	// 	return 0.f;
	// }
	//
	// return distance;
}

//TODO make pawns into quadtree (cuz we can)
//TODO make moves smoother
void Pawn::Move()
{
	const float speed = GetSpeed() * _env->deltaTime;
	if (keyboardButtons.a && GetX() + speed >= 0.f)
	{
		SetDirection(LEFT);
		if (const auto pawns = IsCanMove(); pawns.empty())
		{
			MoveX(-std::floor(speed));
		}
		else
		{
			const auto getSideDiff = [thisLeftSide = this->GetX()](const std::shared_ptr<BaseObj>& pawn) -> float
			{ return thisLeftSide - pawn->GetRightSide(); };

			constexpr float padding = 1.f;
			if (const float distance = FindNearestDistance(pawns, getSideDiff) - padding; distance > 0.f)
			{
				MoveX(-std::floor(distance));
			}
		}
	}
	else if (keyboardButtons.d && GetRightSide() + speed < static_cast<float>(_env->windowWidth))
	{
		SetDirection(RIGHT);
		if (const auto pawns = IsCanMove(); pawns.empty())
		{
			MoveX(std::floor(speed));
		}
		else
		{
			auto getSideDiff = [thisRightSide = this->GetRightSide()](const std::shared_ptr<BaseObj>& pawn) -> float
			{ return pawn->GetX() - thisRightSide; };

			constexpr float padding = 1.f;
			if (const float distance = FindNearestDistance(pawns, getSideDiff) - padding; distance > 0.f)
			{
				MoveX(std::floor(distance));
			}
		}
	}
	else if (keyboardButtons.w && GetY() + speed >= 0.0f)
	{
		SetDirection(UP);
		if (const auto pawns = IsCanMove(); pawns.empty())
		{
			MoveY(-std::floor(speed));
		}
		else
		{
			const auto& getSideDiff = [thisTopSide = this->GetY()](const std::shared_ptr<BaseObj>& pawn) -> float
			{ return pawn->GetBottomSide() - thisTopSide; };

			constexpr float padding = 1.f;
			if (const float distance = FindNearestDistance(pawns, getSideDiff) - padding; distance > 0.f)
			{
				MoveY(-std::floor(distance));
			}
		}
	}
	else if (keyboardButtons.s && GetBottomSide() + speed < static_cast<float>(_env->windowHeight))
	{
		SetDirection(DOWN);
		if (const auto pawns = IsCanMove(); pawns.empty())
		{
			MoveY(std::floor(speed));
		}
		else
		{
			const auto getSideDiff = [thisBottomSide =
											  this->GetBottomSide()](const std::shared_ptr<BaseObj>& pawn) -> float
			{ return pawn->GetY() - thisBottomSide; };

			constexpr float padding = 1.f;
			if (const float distance = FindNearestDistance(pawns, getSideDiff) - padding; distance > 0.f)
			{
				MoveY(std::floor(distance));
			}
		}
	}
}

void Pawn::KeyboardEvensHandlers(Uint32 eventType, SDL_Keycode key) {}

Direction Pawn::GetDirection() const { return _direction; }

void Pawn::SetDirection(const Direction direction) { _direction = direction; }

float Pawn::GetBulletWidth() const { return _bulletWidth; }

void Pawn::SetBulletWidth(const float bulletWidth) { _bulletWidth = bulletWidth; }

float Pawn::GetBulletHeight() const { return _bulletHeight; }

void Pawn::SetBulletHeight(const float bulletHeight) { _bulletHeight = bulletHeight; }

float Pawn::GetBulletSpeed() const { return _bulletSpeed; }

void Pawn::SetBulletSpeed(const float bulletSpeed) { _bulletSpeed = bulletSpeed; }
