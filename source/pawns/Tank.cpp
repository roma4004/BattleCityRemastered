#include "../../headers/pawns/Tank.h"
#include "../../headers/EventSystem.h"
#include "../../headers/behavior/ShootingBeh.h"
#include "../../headers/utils/PixelUtils.h"

Tank::Tank(const ObjRectangle& rect, const int color, const int health, std::shared_ptr<Window> window,
           const Direction direction, const float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
           std::shared_ptr<EventSystem> events, std::unique_ptr<IMoveBeh> moveBeh,
           std::shared_ptr<IShootable> shootingBeh, std::string name, std::string fraction, const GameMode gameMode,
           const int id)
	: Pawn{rect,
	       color,
	       health,
	       std::move(window),
	       direction,
	       speed,
	       allObjects,
	       std::move(events),
	       std::move(moveBeh),
	       id,
	       std::move(name) + std::to_string(id),// TODO: maybe name should be without tankId
	  },
	  _shootingBeh{std::move(shootingBeh)},
	  _gameMode{gameMode},
	  _fraction{std::move(fraction)}
{
	Tank::Subscribe();
}

Tank::~Tank()
{
	Tank::Unsubscribe();
}

void Tank::Subscribe()
{
	_events->AddListener("Draw", _name, [this]() { this->Draw(); });
	_events->AddListener("DrawHealthBar", _name, [this]() { this->DrawHealthBar(); });
}

void Tank::Unsubscribe() const
{
	_events->RemoveListener("Draw", _name);
	_events->RemoveListener("DrawHealthBar", _name);
}

std::string Tank::GetFraction() const { return _fraction; }

void Tank::TakeDamage(const int damage)
{
	if (!_helmet.isActive)
	{
		Pawn::TakeDamage(damage);
	}
}

int Tank::GetTankTier() const { return _tier; }

void Tank::Shot() const
{
	_shootingBeh->Shot();
	_lastTimeFire = std::chrono::system_clock::now();

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const Direction>("ServerSend_" + _name + "Shot", GetDirection());
	}
}

float Tank::GetBulletWidth() const { return _bulletWidth; }

void Tank::SetBulletWidth(const float bulletWidth) { _bulletWidth = bulletWidth; }

float Tank::GetBulletHeight() const { return _bulletHeight; }

void Tank::SetBulletHeight(const float bulletHeight) { _bulletHeight = bulletHeight; }

float Tank::GetBulletSpeed() const { return _bulletSpeed; }

void Tank::SetBulletSpeed(const float bulletSpeed) { _bulletSpeed = bulletSpeed; }

int Tank::GetBulletDamage() const { return _bulletDamage; }

void Tank::SetBulletDamage(const int bulletDamage) { _bulletDamage = bulletDamage; }

double Tank::GetBulletDamageRadius() const { return _bulletDamageRadius; }

void Tank::SetBulletDamageRadius(const double bulletDamageRadius) { _bulletDamageRadius = bulletDamageRadius; }

std::chrono::milliseconds Tank::GetFireCooldown() const { return _fireCooldown; }

void Tank::SetFireCooldown(const std::chrono::milliseconds fireCooldown) { _fireCooldown = fireCooldown; }

inline void Tank::SetPixel(const size_t x, const size_t y, const int color) const
{
	if (x < _window->size.x && y < _window->size.y)
	{
		const size_t rowSize = _window->size.x;
		_window->buffer.get()[y * rowSize + x] = color;
	}
}

void Tank::DrawHealthBar() const
{
	//TODO: fix recenter health bar when pickup star bonus
	if (_helmet.isActive)
	{
		return;
	}

	const auto width = static_cast<unsigned int>(_window->size.x);
	size_t y = static_cast<size_t>(GetY()) - 10;
	for (const size_t maxY = y + 5; y < maxY; ++y)
	{
		size_t x = static_cast<size_t>(GetX()) + 2;
		for (const size_t maxX = x + GetHealth() / 3; x < maxX; ++x)
		{
			const unsigned int tankColor = GetColor();
			if (x < _window->size.x && y < _window->size.y)
			{
				int& targetColor = _window->buffer.get()[y * width + x];
				targetColor = static_cast<int>(
					PixelUtils::BlendPixel(targetColor, PixelUtils::ChangeAlpha(tankColor, 127)));
				SetPixel(x, y, targetColor);
			}
		}
	}
}
