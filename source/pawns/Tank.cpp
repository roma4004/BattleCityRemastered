#include "../../headers/pawns/Tank.h"
#include "../../headers/EventSystem.h"
#include "../../headers/behavior/ShootingBeh.h"
#include "../../headers/utils/PixelUtils.h"

Tank::Tank(const ObjRectangle& rect, const int color, const int health, std::shared_ptr<int[]> windowBuffer,
           UPoint windowSize, const Direction direction, const float speed,
           std::vector<std::shared_ptr<BaseObj>>* allObjects, std::shared_ptr<EventSystem> events,
           std::unique_ptr<IMoveBeh> moveBeh, std::shared_ptr<IShootable> shootingBeh, std::string name,
           std::string fraction, const GameMode gameMode, const int id)
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
	  _id{id},
	  _shootingBeh{std::move(shootingBeh)},
	  _gameMode{gameMode},
	  _name{std::move(name) + std::to_string(id)},// TODO: maybe name should be without tankId
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
	if (_events == nullptr)
	{
		return;
	}

	_events->AddListener("Draw", _name, [this]() { this->Draw(); });
	_events->AddListener("DrawHealthBar", _name, [this]() { this->DrawHealthBar(); });
}

void Tank::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener("Draw", _name);
	_events->RemoveListener("DrawHealthBar", _name);
}

int Tank::GetId() const { return _id; }

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

int Tank::GetFireCooldownMs() const { return _fireCooldownMs; }

void Tank::SetFireCooldownMs(const int fireCooldown) { _fireCooldownMs = fireCooldown; }

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
				targetColor = static_cast<int>(
					PixelUtils::BlendPixel(targetColor, PixelUtils::ChangeAlpha(tankColor, 127)));
				SetPixel(x, y, targetColor);
			}
		}
	}
}
