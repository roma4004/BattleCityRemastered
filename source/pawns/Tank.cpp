#include "../../headers/pawns/Tank.h"
#include "../../headers/behavior/ShootingBeh.h"
#include "../../headers/utils/PixelUtils.h"

Tank::Tank(const ObjRectangle& rect, const int color, const int health, std::shared_ptr<int[]> windowBuffer,
           UPoint windowSize, const Direction direction, const float speed,
           std::vector<std::shared_ptr<BaseObj>>* allObjects, std::shared_ptr<EventSystem> events,
           std::unique_ptr<IMoveBeh> moveBeh, std::shared_ptr<IShootable> shootingBeh, std::string name,
           std::string fraction, const int tankId)
	: Pawn{rect,
	       color,
	       health,
	       std::move(windowBuffer),
	       std::move(windowSize),
	       direction,
	       speed,
	       allObjects,
	       std::move(events),
	       std::move(moveBeh)},
	  _shootingBeh{std::move(shootingBeh)},
	  _tankId{tankId},
	  _name{std::move(name) + std::to_string(tankId)},// TODO: maybe name should be without tankId
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

int Tank::GetTankId() const { return _tankId; }

void Tank::DrawHealthBar() const
{
	if (_windowBuffer == nullptr)
	{
		return;
	}

	//TODO: fix recenter health bar when pickup star bonus
	if (_isActiveHelmet)
	{
		return;
	}

	const auto width = static_cast<unsigned int>(_windowSize.x);
	size_t y = static_cast<size_t>(GetY()) - 10;
	for (const size_t maxY = y + 5; y < maxY; ++y)
	{
		size_t x = static_cast<size_t>(GetX()) + 2;
		for (const size_t maxX = x + GetHealth() / 3; x < maxX; ++x)
		{
			const unsigned int tankColor = GetColor();
			if (x < _windowSize.x && y < _windowSize.y)
			{
				int& targetColor = _windowBuffer.get()[y * width + x];
				targetColor = static_cast<int>(PixelUtils::BlendPixel(targetColor,
				                                                      PixelUtils::ChangeAlpha(tankColor, 127)));
				SetPixel(x, y, targetColor);
			}
		}
	}
}
