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
	_events->AddListener<const GameMode>("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		_gameMode = newGameMode;
	});
}

void BulletPool::Unsubscribe() const
{
	_events->RemoveListener("GameModeChangedTo", _name);
}

std::shared_ptr<BaseObj> BulletPool::GetBullet(const ObjRectangle& rect, const int damage, const double aoeRadius,
                                               const int color, const int health, const Direction direction,
                                               const float speed, std::string author, std::string fraction)
{
	if (_bullets.empty())
	{
		auto bullet = std::shared_ptr<Bullet>(
				new Bullet{rect, damage, aoeRadius, color, health, _window, direction, speed, _allObjects, _events,
				           std::move(author), std::move(fraction), _gameMode, _lastId++},
				[this](Bullet* b)
				{
					ReturnBullet(b);
				});

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

void BulletPool::ReturnBullet(BaseObj* bullet)
{
	if (const auto* bulletCast = dynamic_cast<Bullet*>(bullet); bulletCast != nullptr)
	{
		bulletCast->Disable();
		_bullets.emplace(std::shared_ptr<BaseObj>(bullet, [this](BaseObj* b)
		{
			ReturnBullet(b);
		}));

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
