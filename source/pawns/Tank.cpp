#include "../../headers/pawns/Tank.h"
#include "../../headers/EventSystem.h"
#include "../../headers/behavior/ShootingBeh.h"
#include "../../headers/utils/PixelUtils.h"
#include "../../headers/utils/TimeUtils.h"

Tank::Tank(const ObjRectangle& rect, const int color, const int health, std::shared_ptr<Window> window,
           const Direction direction, const float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
           std::shared_ptr<EventSystem> events, std::unique_ptr<IMoveBeh> moveBeh,
           std::shared_ptr<IShootable> shootingBeh, std::string name, std::string fraction, const GameMode gameMode,
           const int id, const int tier)
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
	       std::move(fraction),
	       tier
	  },
	  _shootingBeh{std::move(shootingBeh)},
	  _gameMode{gameMode}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	Tank::Subscribe();

	_events->EmitEvent(_name + "_Spawn");

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string&>("ServerSend_TankSpawn", _name);//TODO: write this replicate
	}
}

Tank::~Tank()
{
	Tank::Unsubscribe();

	_events->EmitEvent(_name + "_Died");

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string&>("ServerSend_TankDied", _name);
	}
}

void Tank::Subscribe()
{
	_events->AddListener("Draw", _name, [this]() { this->Draw(); });
	_events->AddListener("DrawHealthBar", _name, [this]() { this->DrawHealthBar(); });

	_gameMode == PlayAsClient ? SubscribeAsClient() : SubscribeAsHost();

	Tank::SubscribeBonus();
}

void Tank::SubscribeAsHost()
{
	_events->AddListener<const float>("TickUpdate", _name, [this](const float deltaTime)
	{
		if (_timer.isActive && TimeUtils::IsCooldownFinish(_timer.activateTime, _timer.cooldown))
		{
			_timer.isActive = false;
		}

		if (_helmet.isActive && TimeUtils::IsCooldownFinish(_helmet.activateTime, _helmet.cooldown))
		{
			_helmet.isActive = false;

			if (_gameMode == PlayAsHost)
			{
				_events->EmitEvent<const std::string&>("ServerSend_OnHelmetDeactivate", _name);
			}
		}

		if (!this->_timer.isActive)
		{
			this->TickUpdate(deltaTime);
		}
	});
}

void Tank::SubscribeAsClient()
{
	_events->AddListener<const FPoint, const Direction>(
			"ClientReceived_" + _name /*TODO: + std::to_string(GetId())*/ + "Pos", _name,
			[this](const FPoint newPos, const Direction direction)
			{
				this->SetPos(newPos);
				this->SetDirection(direction);
			});

	_events->AddListener<const Direction>(
			"ClientReceived_" + _name + "Shot", _name, [this](const Direction direction)
			{
				this->SetDirection(direction);
				this->Shot();
			});

	_events->AddListener<const int>("ClientReceived_" + _name + "Health", _name, [this](const int health)
	{
		this->SetHealth(health);
	});

	_events->AddListener<const std::string&>(
			"ClientReceived_" + _name + "TankDied", _name, [this](const std::string&/* whoDied*/)
			{
				this->SetIsAlive(false);
			});

	_events->AddListener("ClientReceived_" + _name + "OnHelmetActivate", _name, [this]()
	{
		this->_helmet.isActive = true;
	});

	_events->AddListener("ClientReceived_" + _name + "OnHelmetDeactivate", _name, [this]()
	{
		this->_helmet.isActive = false;
	});

	_events->AddListener("ClientReceived_" + _name + "OnStar", _name, [this]()
	{
		this->OnBonusStar(_name, _fraction);
	});
}

void Tank::SubscribeBonus()
{
	_events->AddListener<const std::string&, const std::string&, const std::chrono::milliseconds>(
			"BonusTimer", _name,
			[this](const std::string& /*author*/, const std::string& fraction, const std::chrono::milliseconds duration)
			{
				this->OnBonusTimer(fraction, duration);
			});

	_events->AddListener<const std::string&, const std::string&, const std::chrono::milliseconds>(
			"BonusHelmet", _name,
			[this](const std::string& author, const std::string& fraction, const std::chrono::milliseconds duration)
			{
				this->OnBonusHelmet(author, fraction, duration);
			});

	_events->AddListener<const std::string&, const std::string&>(
			"BonusGrenade", _name, [this](const std::string& author, const std::string& fraction)
			{
				this->OnBonusGrenade(author, fraction);
			});

	_events->AddListener<const std::string&, const std::string&>(
			"BonusStar", _name, [this](const std::string& author, const std::string& fraction)
			{
				this->OnBonusStar(author, fraction);
			});
}

void Tank::Unsubscribe() const
{
	_events->RemoveListener("Draw", _name);
	_events->RemoveListener("DrawHealthBar", _name);

	_gameMode == PlayAsClient ? UnsubscribeAsClient() : UnsubscribeAsHost();

	Tank::UnsubscribeBonus();
}

void Tank::UnsubscribeAsHost() const
{
	_events->RemoveListener<const float>("TickUpdate", _name);
}

void Tank::UnsubscribeAsClient() const
{
	_events->RemoveListener<const FPoint, const Direction>("ClientReceived_" + _name + "Pos", _name);
	_events->RemoveListener<const Direction>("ClientReceived_" + _name + "Shot", _name);
	_events->RemoveListener<const int>("ClientReceived_" + _name + "Health", _name);
	_events->RemoveListener<const std::string&>("ClientReceived_" + _name + "TankDied", _name);

	_events->RemoveListener("ClientReceived_" + _name + "OnHelmetActivate", _name);
	_events->RemoveListener("ClientReceived_" + _name + "OnHelmetDeactivate", _name);
	_events->RemoveListener("ClientReceived_" + _name + "OnStar", _name);
}

void Tank::UnsubscribeBonus() const
{
	_events->RemoveListener<const std::string&, const std::string&, const std::chrono::milliseconds>("BonusTimer", _name);
	_events->RemoveListener<const std::string&, const std::string&, const std::chrono::milliseconds>("BonusHelmet", _name);
	_events->RemoveListener<const std::string&, const std::string&>("BonusGrenade", _name);
	_events->RemoveListener<const std::string&, const std::string&>("BonusStar", _name);
}

void Tank::TakeDamage(const int damage)
{
	if (!_helmet.isActive)
	{
		Pawn::TakeDamage(damage);
	}
}

int Tank::GetTier() const { return _tier; }

void Tank::Shot() const
{
	_shootingBeh->Shot();
	_lastTimeFire = std::chrono::system_clock::now();

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string&, const Direction>("ServerSend_Shot", _name, GetDirection());
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

void Tank::OnBonusTimer(const std::string& fraction, const std::chrono::milliseconds duration)
{
	if (fraction != _fraction)
	{
		const auto cooldown = _timer.cooldown += duration;
		_timer = {true, cooldown, std::chrono::system_clock::now()};
	}
}

void Tank::OnBonusHelmet(const std::string& author, const std::string& fraction,
                         const std::chrono::milliseconds duration)
{
	if (fraction == _fraction && author == _name)
	{
		const auto cooldown = _helmet.cooldown += duration;
		_helmet = {true, cooldown, std::chrono::system_clock::now()};

		if (_gameMode == PlayAsHost)
		{
			_events->EmitEvent<const std::string&>("ServerSend_OnHelmetActivate", author);
		}
	}
}

void Tank::OnBonusGrenade(const std::string& author, const std::string& fraction)
{
	if (fraction != _fraction)
	{
		TakeDamage(GetHealth());
	}
}

void Tank::OnBonusStar(const std::string& author, const std::string& fraction)
{
	if (fraction == _fraction && author == _name)
	{
		SetHealth(GetHealth() + 50);
		if (_tier > 4)
		{
			return;
		}

		++_tier;

		SetSpeed(GetSpeed() * 1.10f);
		SetBulletSpeed(GetBulletSpeed() * 1.10f);
		SetBulletDamage(GetBulletDamage() + 15);
		SetFireCooldown(GetFireCooldown() - std::chrono::milliseconds{150});
		SetBulletDamageRadius(GetBulletDamageRadius() * 1.25f);

		if (_gameMode == PlayAsHost)
		{
			_events->EmitEvent<const std::string&>("ServerSend_OnStar", author);
		}
	}
}
