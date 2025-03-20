#include "../../headers/components/BonusSpawner.h"
#include "../../headers/application/Window.h"
#include "../../headers/bonuses/BonusGrenade.h"
#include "../../headers/bonuses/BonusHelmet.h"
#include "../../headers/bonuses/BonusShovel.h"
#include "../../headers/bonuses/BonusStar.h"
#include "../../headers/bonuses/BonusTank.h"
#include "../../headers/bonuses/BonusTimer.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/BonusType.h"
#include "../../headers/enums/GameMode.h"
#include "../../headers/utils/ColliderUtils.h"
#include "../../headers/utils/TimeUtils.h"

#include <algorithm>
#include <chrono>
#include <limits>
#include <memory>

class BaseObj;
class EventSystem;

BonusSpawner::BonusSpawner(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                           std::shared_ptr<Window> window, const int sideBarWidth, const int bonusSize)
	: _bonusSize{bonusSize},
	  _events{std::move(events)},
	  _window{window},
	  _allObjects{allObjects},
	  _distSpawnPosY{0, static_cast<int>(window->size.y) - bonusSize},
	  _distSpawnPosX{0, static_cast<int>(window->size.x) - sideBarWidth - bonusSize},
	  _distSpawnType{None + 1, lastId - 1},
	  _distRandColor{0, std::numeric_limits<int>::max()},
	  _lastTimeSpawn{std::chrono::system_clock::now()}
{
	std::random_device rd;
	_gen = std::mt19937(
			static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count()) + rd());

	Subscribe();
}

BonusSpawner::~BonusSpawner()
{
	Unsubscribe();
}

void BonusSpawner::Subscribe()
{
	_events->AddListener<const GameMode>("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->_gameMode = newGameMode;

		if (_gameMode == PlayAsClient)
		{
			UnsubscribeAsHost();
			SubscribeAsClient();
		}
		else
		{
			SubscribeAsHost();
			UnsubscribeAsClient();
		}
	});

	_events->AddListener("Reset", _name, [this]()
	{
		this->_lastSpawnId = -1;
	});

	_gameMode == PlayAsClient ? SubscribeAsClient() : SubscribeAsHost();
}

void BonusSpawner::SubscribeAsHost()
{
	_events->AddListener<const float>("TickUpdate", _name, [this](const float deltaTime)
	{
		this->TickUpdate(deltaTime);
	});
}

void BonusSpawner::SubscribeAsClient()
{
	_events->AddListener<const FPoint, const BonusType, const int>(
			"ClientReceived_BonusSpawn", _name, [this](const FPoint pos, const BonusType type, const int id)
			{
				const auto size = static_cast<float>(_bonusSize);
				const int color = _distRandColor(_gen);
				ObjRectangle rect{.x = pos.x, .y = pos.y, .w = size, .h = size};
				SpawnBonus(std::move(rect), color, type, id);
			});
}

void BonusSpawner::Unsubscribe() const
{
	_events->RemoveListener<const GameMode>("GameModeChangedTo", _name);

	_events->RemoveListener("Reset", _name);

	_gameMode == PlayAsClient ? UnsubscribeAsClient() : UnsubscribeAsHost();
}

void BonusSpawner::UnsubscribeAsHost() const
{
	_events->RemoveListener<const float>("TickUpdate", _name);
}

void BonusSpawner::UnsubscribeAsClient() const
{
	_events->RemoveListener<const FPoint, const BonusType, const int>("ClientReceived_BonusSpawn", _name);
}

void BonusSpawner::TickUpdate(const float /*deltaTime*/)
{
	if (TimeUtils::IsCooldownFinish(_lastTimeSpawn, _cooldownBonusSpawn))
	{
		const auto size = static_cast<float>(_bonusSize);
		const auto x = static_cast<float>(_distSpawnPosX(_gen));
		const auto y = static_cast<float>(_distSpawnPosY(_gen));
		const ObjRectangle rect{.x = x, .y = y, .w = size, .h = size};
		const bool isFreeSpawnSpot = !std::ranges::any_of(*_allObjects, [&rect](const std::shared_ptr<BaseObj>& object)
		{
			return ColliderUtils::IsCollide(rect, object->GetShape());
		});

		if (isFreeSpawnSpot)
		{
			SpawnRandomBonus(std::move(rect));
		}
	}
}

void BonusSpawner::SpawnBonus(ObjRectangle rect, const int color, const BonusType bonusType, const int id)
{
	if (id != -1)
	{
		_lastSpawnId = id;
	}
	else
	{
		++_lastSpawnId;
	}

	switch (bonusType)
	{
		case None:
			break;
		case Timer:
			SpawnBonus<BonusTimer>(std::move(rect), color, _lastSpawnId);
			break;
		case Helmet:
			SpawnBonus<BonusHelmet>(std::move(rect), color, _lastSpawnId);
			break;
		case Grenade:
			SpawnBonus<BonusGrenade>(std::move(rect), color, _lastSpawnId);
			break;
		case Tank:
			SpawnBonus<BonusTank>(std::move(rect), color, _lastSpawnId);
			break;
		case Star:
			SpawnBonus<BonusStar>(std::move(rect), color, _lastSpawnId);
			break;
		case Shovel:
			SpawnBonus<BonusShovel>(std::move(rect), color, _lastSpawnId);
			break;
		default:
			break;
	}
}

void BonusSpawner::SpawnRandomBonus(ObjRectangle rect)
{
	const int color = _distRandColor(_gen);
	const auto bonusType = static_cast<BonusType>(_distSpawnType(_gen));
	SpawnBonus(std::move(rect), color, bonusType);
}

template<typename TBonusType>
void BonusSpawner::SpawnBonus(ObjRectangle rect, const int color, const int id)
{
	constexpr std::chrono::milliseconds lifetime{std::chrono::seconds{15}};
	constexpr std::chrono::milliseconds duration{std::chrono::seconds{15}};

	_allObjects->emplace_back(
			std::make_shared<TBonusType>(std::move(rect), _window, _events, duration, lifetime, color, id, _gameMode));

	_lastTimeSpawn = std::chrono::system_clock::now();
}
