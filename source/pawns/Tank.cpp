#include "../../headers/pawns/Tank.h"
#include "../../headers/ShootingBeh.h"
#include "../../headers/utils/PixelUtils.h"

Tank::Tank(const Rectangle& rect, const int color, const int health, int* windowBuffer, const UPoint windowSize,
           const Direction direction, const float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
           const std::shared_ptr<EventSystem>& events, std::shared_ptr<IMoveBeh> moveBeh,
           std::shared_ptr<BulletPool> bulletPool, std::string name, std::string fraction)
	: Pawn{rect,
	       color,
	       health,
	       windowBuffer,
	       windowSize,
	       direction,
	       speed,
	       allObjects,
	       events,
	       std::move(moveBeh)},
	  _shootingBeh{std::make_shared<ShootingBeh>(this, windowBuffer, allObjects, events, std::move(bulletPool))},
	  _lastTimeFire{std::chrono::system_clock::now()},
	  _name{std::move(name)},
	  _fraction{std::move(fraction)} {}


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

void Tank::Shot() const { _shootingBeh->Shot(); }

float Tank::GetBulletWidth() const { return _bulletWidth; }

void Tank::SetBulletWidth(const float bulletWidth) { _bulletWidth = bulletWidth; }

float Tank::GetBulletHeight() const { return _bulletHeight; }

void Tank::SetBulletHeight(const float bulletHeight) { _bulletHeight = bulletHeight; }

float Tank::GetBulletSpeed() const { return _bulletSpeed; }

void Tank::SetBulletSpeed(const float bulletSpeed) { _bulletSpeed = bulletSpeed; }

int Tank::GetBulletDamage() const { return _bulletDamage; }

void Tank::SetBulletDamage(const int bulletDamage) { _bulletDamage = bulletDamage; }

double Tank::GetBulletDamageAreaRadius() const { return _bulletDamageAreaRadius; }

void Tank::SetBulletDamageAreaRadius(const double bulletDamageAreaRadius)
{
	_bulletDamageAreaRadius = bulletDamageAreaRadius;
}

int Tank::GetFireCooldownMs() const { return _fireCooldownMs; }

void Tank::SetFireCooldownMs(const int fireCooldown) { _fireCooldownMs = fireCooldown; }

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
