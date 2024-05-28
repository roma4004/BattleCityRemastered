#include <cmath>

#include "../headers/Bullet.h"
#include "../headers/Pawn.h"

Pawn::Pawn(const Rectangle& rect, const int color, const float speed, const int health, int* windowBuffer,
		   const UPoint windowSize, std::vector<std::shared_ptr<BaseObj>>* allPawns,
		   std::shared_ptr<EventSystem> events)
	: BaseObj{rect, color, speed, health}, _windowSize{windowSize}, _windowBuffer{windowBuffer},
	  _events{std::move(events)}, _allPawns{allPawns}
{
}

Pawn::~Pawn() = default;

void Pawn::SetPixel(const size_t x, const size_t y, const int color) const
{
	if (x < _windowSize.x && y < _windowSize.y)
	{
		const size_t rowSize = _windowSize.x;
		_windowBuffer[y * rowSize + x] = color;
	}
}

void Pawn::Draw() const
{
	int y = static_cast<int>(GetY());
	for (const int maxY = y + static_cast<int>(GetHeight()); y < maxY; ++y)
	{
		int x = static_cast<int>(GetX());
		for (const int maxX = x + static_cast<int>(GetWidth()); x < maxX; ++x)
		{
			SetPixel(x, y, GetColor());
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

std::list<std::weak_ptr<BaseObj>> Pawn::IsCanMove(const float deltaTime)
{
	const Direction direction = GetDirection();
	float speedX = GetSpeed() * deltaTime;
	float speedY = GetSpeed() * deltaTime;

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
	const auto thisNextPosRect =
			Rectangle{this->GetX() + speedX, this->GetY() + speedY, this->GetWidth(), this->GetHeight()};
	for (std::shared_ptr<BaseObj>& pawn: *_allPawns)
	{
		if (this == pawn.get())
		{
			continue;
		}

		if (IsCollideWith(thisNextPosRect, pawn->GetShape()))
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

void Pawn::TickUpdate(const float deltaTime)
{
	Move(deltaTime);

	if (keyboardButtons.shot)
	{
		Shot();
		keyboardButtons.shot = false;
	}
}

void Pawn::Shot()
{
	const Direction direction = GetDirection();
	const FPoint tankHalf = {GetWidth() / 2.f, GetHeight() / 2.f};
	const float tankX = GetX();
	const float tankY = GetY();
	const float tankRightX = GetRightSide();
	const float tankBottomY = GetBottomSide();
	const FPoint tankCenter = {tankX + tankHalf.x, tankY + tankHalf.y};

	const float bulletWidth = GetBulletWidth();
	const float bulletHeight = GetBulletHeight();
	const FPoint bulletHalf = {bulletWidth / 2.f, bulletHeight / 2.f};
	Rectangle bulletRect{};

	if (direction == UP && tankY - bulletHeight >= 0.f)//TODO: rewrite check with zero to use epsilon
	{
		bulletRect = {tankCenter.x - bulletHalf.x, tankCenter.y - tankHalf.y - bulletHalf.y, bulletWidth, bulletHeight};
	}
	else if (direction == DOWN && tankBottomY + bulletHeight <= static_cast<float>(_windowSize.y))
	{
		bulletRect = {tankCenter.x - bulletHalf.x, tankBottomY + bulletHalf.y, bulletWidth, bulletHeight};
	}
	else if (direction == LEFT && tankX - bulletWidth >= 0.f)//TODO: rewrite check with zero to use epsilon
	{
		bulletRect = {tankX - bulletHalf.x, tankCenter.y - bulletHalf.y, bulletWidth, bulletHeight};
	}
	else if (direction == RIGHT && tankRightX + bulletHalf.x + bulletWidth <= static_cast<float>(_windowSize.x))
	{
		bulletRect = {tankRightX + bulletHalf.x, tankCenter.y - bulletHalf.y, bulletWidth, bulletHeight};
	}
	else
	{
		keyboardButtons.shot = false;
		return;
	}

	constexpr int color = 0xffffff;
	const float speed = GetBulletSpeed();
	constexpr int health = 1;
	_allPawns->emplace_back(std::make_shared<Bullet>(bulletRect, color, speed, direction, health, _windowBuffer,
													 _windowSize, _allPawns, _events));
}

inline float Distance(const FPoint a, const FPoint b)
{
	return static_cast<float>(std::sqrt(std::pow(b.x - a.x, 2) + std::pow(b.y - a.y, 2)));
}

float Pawn::FindNearestDistance(const std::list<std::weak_ptr<BaseObj>>& pawns,
								const std::function<float(const std::shared_ptr<BaseObj>&)>& getNearestSide) const
{
	float nearestDist = static_cast<float>(_windowSize.x * _windowSize.y);
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
void Pawn::Move(const float deltaTime)
{
	const float speed = GetSpeed() * deltaTime;
	if (keyboardButtons.a && GetX() - speed >= 0.f)
	{
		SetDirection(LEFT);
		if (const auto pawns = IsCanMove(deltaTime); pawns.empty())
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
	else if (keyboardButtons.d && GetRightSide() + speed < static_cast<float>(_windowSize.x))
	{
		SetDirection(RIGHT);
		if (const auto pawns = IsCanMove(deltaTime); pawns.empty())
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
	else if (keyboardButtons.w && GetY() - speed >= 0.0f)
	{
		SetDirection(UP);
		if (const auto pawns = IsCanMove(deltaTime); pawns.empty())
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
	else if (keyboardButtons.s && GetBottomSide() + speed < static_cast<float>(_windowSize.y))
	{
		SetDirection(DOWN);
		if (const auto pawns = IsCanMove(deltaTime); pawns.empty())
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

Direction Pawn::GetDirection() const { return _direction; }

void Pawn::SetDirection(const Direction direction) { _direction = direction; }

float Pawn::GetBulletWidth() const { return _bulletWidth; }

void Pawn::SetBulletWidth(const float bulletWidth) { _bulletWidth = bulletWidth; }

float Pawn::GetBulletHeight() const { return _bulletHeight; }

void Pawn::SetBulletHeight(const float bulletHeight) { _bulletHeight = bulletHeight; }

float Pawn::GetBulletSpeed() const { return _bulletSpeed; }

void Pawn::SetBulletSpeed(const float bulletSpeed) { _bulletSpeed = bulletSpeed; }
