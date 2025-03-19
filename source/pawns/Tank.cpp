#include "../../headers/pawns/Tank.h"
#include "../../headers/application/Window.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/GameMode.h"
#include "../../headers/interfaces/IMoveBeh.h"
#include "../../headers/interfaces/IShootable.h"
#include "../../headers/utils/PixelUtils.h"
#include "../../headers/utils/TimeUtils.h"

Tank::Tank(const ObjRectangle& rect, const int color, const int health, std::shared_ptr<Window> window,
           const Direction dir, const float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
           std::shared_ptr<EventSystem> events, std::unique_ptr<IMoveBeh> moveBeh,
           std::shared_ptr<IShootable> shootingBeh, std::string name, std::string fraction, const GameMode gameMode,
           const int id, const int tier)
	: Pawn{rect,
	       color,
	       health,
	       std::move(window),
	       dir,
	       speed,
	       allObjects,
	       std::move(events),
	       std::move(moveBeh),
	       id,
	       std::move(name) + std::to_string(id),// TODO: maybe name should be without tankId
	       std::move(fraction),
	       tier,
	       gameMode
	  },
	  _shootingBeh{std::move(shootingBeh)}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	Tank::Subscribe();

	_events->EmitEvent<const std::string&>("TankSpawn", _name);
}

Tank::~Tank()
{
	Tank::Unsubscribe();

	_events->EmitEvent<const std::string&>("TankDied", _name);
}

void Tank::Subscribe()
{
	_events->AddListener("DrawHealthBar", _name, [this]() { this->DrawHealthBar(); });

	if (_gameMode == PlayAsClient)
	{
		SubscribeAsClient();
	}

	SubscribeBonus();
}

void Tank::SubscribeAsClient()
{
	_events->AddListener<const Direction>("ClientReceived_" + _name + "Shot", _name, [this](const Direction dir)
	{
		this->SetDirection(dir);
		this->Shot();
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
	_events->RemoveListener("DrawHealthBar", _name);

	if (_gameMode == PlayAsClient)
	{
		UnsubscribeAsClient();
	}

	UnsubscribeBonus();
}

void Tank::UnsubscribeAsClient() const
{
	_events->RemoveListener<const Direction>("ClientReceived_" + _name + "Shot", _name);

	_events->RemoveListener("ClientReceived_" + _name + "OnHelmetActivate", _name);
	_events->RemoveListener("ClientReceived_" + _name + "OnHelmetDeactivate", _name);
	_events->RemoveListener("ClientReceived_" + _name + "OnStar", _name);
}

void Tank::UnsubscribeBonus() const
{
	_events->RemoveListener<const std::string&, const std::string&, const std::chrono::milliseconds>(
			"BonusTimer", _name);
	_events->RemoveListener<const std::string&, const std::string&, const std::chrono::milliseconds>(
			"BonusHelmet", _name);
	_events->RemoveListener<const std::string&, const std::string&>("BonusGrenade", _name);
	_events->RemoveListener<const std::string&, const std::string&>("BonusStar", _name);
}

void Tank::TickUpdate(const float deltaTime)
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
}

void Tank::TakeDamage(const int damage)
{
	if (!_helmet.isActive)
	{
		Pawn::TakeDamage(damage);

		if (_gameMode == PlayAsHost)
		{
			_events->EmitEvent<const std::string&, const int>("ServerSend_Health", _name, GetHealth());
		}
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

float Tank::GetBulletWidth() const { return _bulletSize.x; }

void Tank::SetBulletWidth(const float bulletWidth) { _bulletSize.x = bulletWidth; }

float Tank::GetBulletHeight() const { return _bulletSize.y; }

void Tank::SetBulletHeight(const float bulletHeight) { _bulletSize.y = bulletHeight; }

float Tank::GetBulletSpeed() const { return _bulletSpeed; }

void Tank::SetBulletSpeed(const float bulletSpeed) { _bulletSpeed = bulletSpeed; }

int Tank::GetBulletDamage() const { return _bulletDamage; }

void Tank::SetBulletDamage(const int bulletDamage) { _bulletDamage = bulletDamage; }

double Tank::GetBulletDamageRadius() const { return _bulletDamageRadius; }

void Tank::SetBulletDamageRadius(const double bulletDamageRadius) { _bulletDamageRadius = bulletDamageRadius; }


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

void Tank::OnBonusGrenade(const std::string& /*author*/, const std::string& fraction)
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

		_speed *= 1.10f;
		_bulletSpeed *= 1.10f;
		_bulletDamage += 15;
		_fireCooldown -= std::chrono::milliseconds{150};
		_bulletDamageRadius *= 1.25f;

		if (_gameMode == PlayAsHost)
		{
			_events->EmitEvent<const std::string&>("ServerSend_OnStar", author);
		}
	}
}

void Tank::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&, const std::string&>(
			"Statistics_TankHit", _name, author, fraction);

	if (GetHealth() < 1)
	{
		//TODO: move to event from statistic when last tank died
		_events->EmitEvent<const std::string&, const std::string&, const std::string&>(
				"Statistics_TankDied", _name, author, fraction);
	}
}
