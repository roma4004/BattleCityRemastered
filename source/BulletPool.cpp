#include "../headers/BulletPool.h"

#include "../headers/GameMode.h"
#include "../headers/pawns/Bullet.h"

BulletPool::BulletPool(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                       const UPoint windowSize, int* windowBuffer, GameMode* currentGameMode)
	: _events{std::move(events)},
	  _name{"BulletPool"},
	  _currentMode{currentGameMode},
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

void BulletPool::Subscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	// _events->AddListener<const GameMode>("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	// {
	// 	this->_currentMode = newGameMode;
	// });
}

void BulletPool::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	// _events->RemoveListener("GameModeChangedTo", _name);
}

std::shared_ptr<BaseObj> BulletPool::GetBullet(const ObjRectangle& rect, int damage, double aoeRadius, int color,
                                               int health, Direction direction, float speed, std::string author,
                                               std::string fraction)

{
	const bool isNetworkControlled = *_currentMode == PlayAsClient;
	if (_bullets.empty())
	{
		auto bullet = std::make_shared<Bullet>(rect, damage, aoeRadius, color, health, _windowBuffer, _windowSize,
		                                       direction, speed, _allObjects, _events, std::move(author), std::move(fraction),
		                                       lastId++, isNetworkControlled);

		return bullet;
	}

	std::shared_ptr<BaseObj> bulletAsBase = _bullets.front();
	_bullets.pop();
	if (const auto bullet = dynamic_cast<Bullet*>(bulletAsBase.get()); bullet != nullptr)
	{
		bullet->Reset(rect, damage, aoeRadius, color, speed, direction, health, std::move(author), std::move(fraction),
		              isNetworkControlled);
	}

	return bulletAsBase;
}

void BulletPool::ReturnBullet(std::shared_ptr<BaseObj> bullet)
{
	if (const auto bulletCast = dynamic_cast<Bullet*>(bullet.get()); bulletCast != nullptr)
	{
		bulletCast->Disable();
		_bullets.emplace(std::move(bullet));
		_events->EmitEvent<const int>("Bullet_Dispose", bulletCast->GetBulletId());
	}
}

void BulletPool::Clear()
{
	while (!_bullets.empty())
	{
		_bullets.pop();
	}

	lastId = 0;
}
