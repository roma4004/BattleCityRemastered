#include "components/BonusSpawner.h"
#include "Point.h"
#include "components/EventSystem.h"
#include "entities/bonuses/BonusCaliber.h"
#include "entities/bonuses/BonusGrenade.h"
#include "entities/bonuses/BonusHelmet.h"
#include "entities/bonuses/BonusShovel.h"
#include "entities/bonuses/BonusStar.h"
#include "entities/bonuses/BonusTank.h"
#include "entities/bonuses/BonusTimer.h"
#include "enums/BonusType.h"
#include "enums/GameMode.h"
#include "utils/ColliderUtils.h"
#include "utils/TimeUtils.h"
#include "utils/UuidUtils.h"
#include <algorithm>
#include <chrono>
#include <limits>
#include <memory>

class BaseObj;
class EventSystem;

BonusSpawner::BonusSpawner(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                           const UPoint windowSize, const int sideBarWidth, const int bonusSize)
	: _bonusSize{bonusSize},
	  _events{std::move(events)},
	  _allObjects{allObjects},
	  _distSpawnPosY{0, static_cast<int>(windowSize.y) - bonusSize},
	  _distSpawnPosX{0, static_cast<int>(windowSize.x) - sideBarWidth - bonusSize},
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

	_gameMode == PlayAsClient ? SubscribeAsClient() : SubscribeAsHost();
}

void BonusSpawner::SubscribeAsHost()
{
	_events->AddListener<const float>("TickUpdate", _name, [this](const float /*deltaTime*/)
	{
		this->Update();
	});
}

void BonusSpawner::SubscribeAsClient()
{
	_events->AddListener<const FPoint, const BonusType, const buuid&>(
			"ClientReceived_BonusSpawn", _name, [this](const FPoint pos, const BonusType type, const buuid& uuid)
			{
				const auto size = static_cast<float>(_bonusSize);
				const int color = _distRandColor(_gen);
				const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = size, .h = size};
				SpawnBonus(rect, color, type, uuid);
			});
}

void BonusSpawner::Unsubscribe() const
{
	_events->RemoveListener<const GameMode>("GameModeChangedTo", _name);

	_gameMode == PlayAsClient ? UnsubscribeAsClient() : UnsubscribeAsHost();
}

void BonusSpawner::UnsubscribeAsHost() const
{
	_events->RemoveListener<const float>("TickUpdate", _name);
}

void BonusSpawner::UnsubscribeAsClient() const
{
	_events->RemoveListener<const FPoint, const BonusType, const buuid&>("ClientReceived_BonusSpawn", _name);
}

void BonusSpawner::Update()
{
	if (TimeUtils::IsCooldownFinish(_lastTimeSpawn, _cooldownBonusSpawn))//TODO: extract to timer manager
	{
		const auto size = static_cast<float>(_bonusSize);
		const auto x = static_cast<float>(_distSpawnPosX(_gen));
		const auto y = static_cast<float>(_distSpawnPosY(_gen));
		const ObjRectangle rect{.x = x, .y = y, .w = size, .h = size};
		const bool isFreeSpawnSpot = !std::ranges::any_of(*_allObjects, [&rect](const std::shared_ptr<BaseObj>& object)
		{
			if (object == nullptr)
			{
				return false;
			}

			return ColliderUtils::IsCollide(rect, object->GetRect());
		});

		if (isFreeSpawnSpot)
		{
			SpawnRandomBonus(rect);
		}
	}
}

void BonusSpawner::SpawnBonus(const ObjRectangle rect, const int color, const BonusType type, buuid uuid)
{
	constexpr milliseconds lifetime{std::chrono::seconds{15}};
	constexpr milliseconds duration{std::chrono::seconds{15}};

	if (uuid == UuidUtils::GetNilUuid())
	{
		uuid = UuidUtils::GetRandomUuid();
	}

	std::shared_ptr<Bonus> bonus{nullptr};

	switch (type)
	{
		case Timer:
			bonus = std::make_shared<BonusTimer>(rect, _events, duration, lifetime, color, uuid, _gameMode);
			break;
		case Helmet:
			bonus = std::make_shared<BonusHelmet>(rect, _events, duration, lifetime, color, uuid, _gameMode);
			break;
		case Grenade:
			bonus = std::make_shared<BonusGrenade>(rect, _events, duration, lifetime, color, uuid, _gameMode);
			break;
		case Tank:
			bonus = std::make_shared<BonusTank>(rect, _events, duration, lifetime, color, uuid, _gameMode);
			break;
		case Star:
			bonus = std::make_shared<BonusStar>(rect, _events, duration, lifetime, color, uuid, _gameMode);
			break;
		case Shovel:
			bonus = std::make_shared<BonusShovel>(rect, _events, duration, lifetime, color, uuid, _gameMode);
			break;
		case Caliber:
			bonus = std::make_shared<BonusCaliber>(rect, _events, duration, lifetime, color, uuid, _gameMode);
			break;
		default:
			break;
	}

	if (bonus)
	{
		_allObjects->emplace_back(bonus);
		_lastTimeSpawn = std::chrono::system_clock::now();
	}
}

void BonusSpawner::SpawnRandomBonus(const ObjRectangle rect)
{
	const int color = _distRandColor(_gen);
	const auto bonusType = static_cast<BonusType>(_distSpawnType(_gen));
	SpawnBonus(rect, color, bonusType);
}
