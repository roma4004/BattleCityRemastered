#include "../headers/BulletPool.h"
#include "../headers/EventSystem.h"
#include "../headers/GameMode.h"
#include "../headers/pawns/Bullet.h"

BulletPool::BulletPool(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                       std::shared_ptr<Window> window, const GameMode gameMode)
	: _events{std::move(events)},
	  _name{"BulletPool"},
	  _gameMode{gameMode},
	  _allObjects{allObjects},
	  _window{std::move(window)}
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
		_gameMode = newGameMode;
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
                                               int health, Direction direction, float speed, std::string author,
                                               std::string fraction)
{
	if (_bullets.empty())
	{
		auto bullet = std::make_shared<Bullet>(
				rect, damage, aoeRadius, color, health, _window, direction, speed, _allObjects, _events,
				std::move(author), std::move(fraction), _gameMode, _lastId++);

		return bullet;
	}

	std::shared_ptr<BaseObj> bulletAsBase = _bullets.front();
	_bullets.pop();
	if (auto* bullet = dynamic_cast<Bullet*>(bulletAsBase.get()); bullet != nullptr)
	{
		bullet->Reset(rect, damage, aoeRadius, color, speed, direction, health, std::move(author), std::move(fraction));
	}

	return bulletAsBase;
}

void BulletPool::ReturnBullet(std::shared_ptr<BaseObj> bullet)
{
	if (const auto* bulletCast = dynamic_cast<Bullet*>(bullet.get()); bulletCast != nullptr)
	{
		bulletCast->Disable();
		_bullets.emplace(std::move(bullet));
		_events->EmitEvent<const int>("ServerSend_Dispose", bulletCast->GetId());
	}
}

void BulletPool::Clear()
{
	while (!_bullets.empty())
	{
		_bullets.pop();
	}

	_lastId = 0;
}
