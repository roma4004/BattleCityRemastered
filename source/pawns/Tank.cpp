#include "../../headers/pawns/Tank.h"
#include "../../headers/utils/PixelUtils.h"

Tank::Tank(const Rectangle& rect, const int color, const int health, int* windowBuffer, const UPoint windowSize,
           const Direction direction, const float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
           std::shared_ptr<EventSystem> events, std::shared_ptr<IMoveBeh> moveBeh,
           std::shared_ptr<BulletPool> bulletPool, std::string name, std::string fraction)
	: Pawn{rect,
	       color,
	       health,
	       windowBuffer,
	       windowSize,
	       direction,
	       speed,
	       allObjects,
	       std::move(events),
	       std::move(moveBeh)},
	  _lastTimeFire{std::chrono::system_clock::now()},
	  _bulletPool{std::move(bulletPool)},
	  _name{std::move(name)},
	  _fraction{std::move(fraction)} {}

//Note: {-1.f, -1.f} this is try shooting outside screen
Rectangle Tank::GetBulletStartRect() const
{
	const FPoint tankHalf = {GetWidth() / 2.f, GetHeight() / 2.f};
	const FPoint tankPos = GetPos();
	const float tankRightX = GetRightSide();
	const float tankBottomY = GetBottomSide();
	const FPoint tankCenter = {tankPos.x + tankHalf.x, tankPos.y + tankHalf.y};

	const float bulletWidth = GetBulletWidth();
	const float bulletHeight = GetBulletHeight();
	const FPoint bulletHalf = {bulletWidth / 2.f, bulletHeight / 2.f};
	Rectangle bulletRect = {-1, -1, bulletWidth, bulletHeight};

	if (_direction == UP && tankPos.y - bulletHalf.y >= 0.f)//TODO: rewrite check with zero to use epsilon
	{
		bulletRect.x = tankCenter.x - bulletHalf.x;
		bulletRect.y = tankPos.y - bulletHalf.y;
	}
	else if (_direction == DOWN && tankBottomY + bulletHalf.y <= static_cast<float>(_windowSize.y))
	{
		bulletRect.x = tankCenter.x - bulletHalf.x;
		bulletRect.y = tankBottomY - bulletHalf.y;
	}
	else if (_direction == LEFT && tankPos.x - bulletWidth >= 0.f)//TODO: rewrite check with zero to use epsilon
	{
		bulletRect.x = tankPos.x - bulletHalf.x;
		bulletRect.y = tankCenter.y - bulletHalf.y;
	}
	else if (_direction == RIGHT && tankRightX + bulletHalf.x + bulletWidth <= static_cast<float>(_windowSize.x))
	{
		bulletRect.x = tankRightX - bulletHalf.x;
		bulletRect.y = tankCenter.y - bulletHalf.y;
	}

	return bulletRect;
}

std::string Tank::GetName() const { return _name; }

std::string Tank::GetFraction() const { return _fraction; }

void Tank::TakeDamage(const int damage)
{
	if (!_isActiveHelmet)
	{
		Pawn::TakeDamage(damage);
	}
}

int Tank::GetTankTier() const { return _tier; }

//TODO: extract like shootComponent
void Tank::Shot() const
{
	const Rectangle bulletRect = GetBulletStartRect();
	if (bulletRect.x < 0.f || bulletRect.y < 0.f)
	{
		//Try shooting outside screen
		return;
	}

	constexpr int color = 0xffffff;
	const float speed = GetBulletSpeed();
	constexpr int health = 1;
	_allObjects->emplace_back(_bulletPool->GetBullet(bulletRect, _damage, _bulletDamageAreaRadius, color, health,
	                                                 _windowBuffer, _windowSize, _direction, speed, _allObjects,
	                                                 _events, _name, _fraction));
}

float Tank::GetBulletWidth() const { return _bulletWidth; }

void Tank::SetBulletWidth(const float bulletWidth) { _bulletWidth = bulletWidth; }

float Tank::GetBulletHeight() const { return _bulletHeight; }

void Tank::SetBulletHeight(const float bulletHeight) { _bulletHeight = bulletHeight; }

float Tank::GetBulletSpeed() const { return _bulletSpeed; }

void Tank::SetBulletSpeed(const float bulletSpeed) { _bulletSpeed = bulletSpeed; }

void Tank::DrawHealthBar() const
{
	if (_isActiveHelmet)
	{
		return;
	}

	const unsigned int width = static_cast<unsigned int>(_windowSize.x);
	int y = static_cast<int>(GetY() - 10);
	for (const int maxY = y + 5; y < maxY; ++y)
	{
		int x = static_cast<int>(GetX() + 2);
		for (const int maxX = x + GetHealth() / 3; x < maxX; ++x)
		{
			const unsigned int tankColor = GetColor();
			if (x < _windowSize.x && y < _windowSize.y)
			{
				int& targetColor = _windowBuffer[y * width + x];
				targetColor = static_cast<int>(PixelUtils::BlendPixel(targetColor,
				                                                      PixelUtils::ChangeAlpha(tankColor, 127)));
				SetPixel(x, y, targetColor);
			}
		}
	}
}
