#include "../headers/BulletPool.h"

#include "../headers/GameMode.h"
#include "../headers/pawns/Bullet.h"

BulletPool::BulletPool(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                       const UPoint windowSize, int* windowBuffer)
	: _events{std::move(events)},
	  _name{"BulletPool"},
	  _currentMode{Demo},
	  _allObjects{allObjects},
	  _windowSize{windowSize},
	  _windowBuffer{windowBuffer}
{
	Subscribe();
}

BulletPool::~BulletPool()
{
	Unsubscribe();
}

void BulletPool::Subscribe()
{
	if (_events == nullptr)
	{
		return;
	}

	_events->AddListener<const GameMode>("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->_currentMode = newGameMode;
	});
}

void BulletPool::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener("GameModeChangedTo", _name);
}

std::shared_ptr<BaseObj> BulletPool::GetBullet(const ObjRectangle& rect, int damage, double aoeRadius, int color,
                                               int health, Direction direction, float speed, std::string name,
                                               std::string fraction)

{
	if (_bullets.empty())
	{
		return std::make_shared<Bullet>(rect, damage, aoeRadius, color, health, _windowBuffer, _windowSize, direction,
		                                speed, _allObjects, _events, std::move(name), std::move(fraction), lastId++,
		                                _currentMode == PlayAsClient);
	}

	std::shared_ptr<BaseObj> bulletAsBase = _bullets.front();
	_bullets.pop();
	if (const auto bullet = dynamic_cast<Bullet*>(bulletAsBase.get()); bullet != nullptr)
	{
		bullet->Reset(rect, damage, aoeRadius, color, speed, direction, health, std::move(name), std::move(fraction),
		              _currentMode == PlayAsClient);
	}

	return bulletAsBase;
}

void BulletPool::ReturnBullet(std::shared_ptr<BaseObj> bullet)
{
	if (const auto bulletCast = dynamic_cast<Bullet*>(bullet.get()); bulletCast != nullptr)
	{
		bulletCast->Disable();
		_bullets.emplace(std::move(bullet));
	}
}
