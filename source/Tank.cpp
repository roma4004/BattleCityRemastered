#include "../headers/Tank.h"
#include "../headers/Bullet.h"

Tank::Tank(const Rectangle& rect, const int color, const float speed, const int health, int* windowBuffer,
           const UPoint windowSize, std::vector<std::shared_ptr<BaseObj>>* allPawns,
           std::shared_ptr<EventSystem> events)
	: Pawn{rect, color, health, windowBuffer, windowSize, allPawns, std::move(events),
	       std::make_shared<MoveLikeTankBeh>(windowSize, speed, this, allPawns)} {}

void Tank::Shot()
{
	const Direction direction = _moveBeh->GetDirection();
	const FPoint tankHalf = {GetWidth() / 2.f, GetHeight() / 2.f};
	const FPoint tankPos = GetPos();
	const float tankRightX = GetRightSide();
	const float tankBottomY = GetBottomSide();
	const FPoint tankCenter = {tankPos.x + tankHalf.x, tankPos.y + tankHalf.y};

	const float bulletWidth = GetBulletWidth();
	const float bulletHeight = GetBulletHeight();
	const FPoint bulletHalf = {bulletWidth / 2.f, bulletHeight / 2.f};
	Rectangle bulletRect{0,0, bulletWidth, bulletHeight};

	if (direction == UP && tankPos.y - bulletHeight >= 0.f)//TODO: rewrite check with zero to use epsilon
	{
		bulletRect.x = tankCenter.x - bulletHalf.x;
		bulletRect.y = tankPos.y - bulletHalf.y;
	}
	else if (direction == DOWN && tankBottomY + bulletHeight <= static_cast<float>(_windowSize.y))
	{
		bulletRect.x = tankCenter.x - bulletHalf.x;
		bulletRect.y = tankBottomY - bulletHalf.y;
	}
	else if (direction == LEFT && tankPos.x - bulletWidth >= 0.f)//TODO: rewrite check with zero to use epsilon
	{
		bulletRect.x = tankPos.x - bulletHalf.x;
		bulletRect.y = tankCenter.y - bulletHalf.y;
	}
	else if (direction == RIGHT && tankRightX + bulletHalf.x + bulletWidth <= static_cast<float>(_windowSize.x))
	{
		bulletRect.x = tankRightX - bulletHalf.x;
		bulletRect.y = tankCenter.y - bulletHalf.y;
	}
	else
	{
		keyboardButtons.shot = false;
		return;
	}

	constexpr int color = 0xffffff;
	const float speed = GetBulletSpeed();
	constexpr int health = 1;
	_allPawns->emplace_back(std::make_shared<Bullet>(bulletRect, _damage, color, speed, direction, health,
	                                                 _windowBuffer, _windowSize, _allPawns, _events));
}

float Tank::GetBulletWidth() const { return _bulletWidth; }

void Tank::SetBulletWidth(const float bulletWidth) { _bulletWidth = bulletWidth; }

float Tank::GetBulletHeight() const { return _bulletHeight; }

void Tank::SetBulletHeight(const float bulletHeight) { _bulletHeight = bulletHeight; }

float Tank::GetBulletSpeed() const { return _bulletSpeed; }

void Tank::SetBulletSpeed(const float bulletSpeed) { _bulletSpeed = bulletSpeed; }