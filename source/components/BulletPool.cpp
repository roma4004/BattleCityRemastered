#include "../../headers/components/BulletPool.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/pawns/Bullet.h"

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
	_events->AddListener("Reset", _name, [this]() { Clear(); });

	_events->AddListener<const GameMode>("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		_gameMode = newGameMode;
	});
}

void BulletPool::Unsubscribe() const
{
	_events->RemoveListener("Reset", _name);

	_events->RemoveListener<const GameMode>("GameModeChangedTo", _name);
}

void BulletPool::SpawnBullet(const ObjRectangle& rect, const int damage, const double aoeRadius, const int color,
                             const int health, const Direction dir, const float speed, std::string author,
                             std::string fraction, const int tier)
{
	std::lock_guard<std::mutex> lock(_bulletsMutex);

	if (_bullets.empty())
	{
		std::shared_ptr<BaseObj> bullet =
				std::shared_ptr<Bullet>(
						new Bullet{rect, damage, aoeRadius, color, health, _window, dir, speed, _allObjects, _events,
						           std::move(author), std::move(fraction), _gameMode, _lastId++, tier},
						[this](Bullet* b)
						{
							ReturnBullet(b);
						});

		_allObjects->emplace_back(bullet);

		return;
	}

	std::shared_ptr<BaseObj> bulletAsBase = _bullets.front();
	_bullets.pop();
	if (auto* bullet = dynamic_cast<Bullet*>(bulletAsBase.get()); bullet != nullptr)
	{
		bullet->Reset(
				rect, damage, aoeRadius, color, speed, dir, health, std::move(author), std::move(fraction), tier);
	}

	_allObjects->emplace_back(bulletAsBase);
}

void BulletPool::ReturnBullet(BaseObj* bullet)
{
	if (_isClearing)
	{
		return;
	}

	if (const auto* bulletCast = dynamic_cast<Bullet*>(bullet); bulletCast != nullptr)
	{
		std::lock_guard<std::mutex> lock(_bulletsMutex);

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
	_isClearing = true;

	while (!_bullets.empty())
	{
		_bullets.pop();
	}

	_isClearing = false;
	_lastId = 0;
}

//TODO: write statistics for pickuped bonuses
