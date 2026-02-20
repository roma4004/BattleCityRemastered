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
#include "utils/RandUtils.h"
#include "utils/TimeUtils.h"
#include "utils/UuidUtils.h"
#include <algorithm>

class BaseObj;
class EventSystem;

BonusSpawner::BonusSpawner(const std::shared_ptr<EventSystem>& events,
                           std::vector<std::shared_ptr<BaseObj>>* allObjects, const UPoint windowSize,
                           const int sideBarWidth, const int bonusSize)
	: _events{events},
	  _allObjects{allObjects},
	  _distSpawnPosY{0, static_cast<int>(windowSize.y) - bonusSize},
	  _distSpawnPosX{0, static_cast<int>(windowSize.x) - sideBarWidth - bonusSize},
	  _distSpawnType{static_cast<int>(BonusType::None) + 1, static_cast<int>(BonusType::lastId) - 1},
	  _distRandColor{0, std::numeric_limits<int>::max()},
	  _lastTimeSpawn{std::chrono::system_clock::now()},
	  _bonusSize{bonusSize}
{
	Subscribe();
}

BonusSpawner::~BonusSpawner()
{
	Unsubscribe();
}

void BonusSpawner::Subscribe()
{
	_events->AddListener("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->_gameMode = newGameMode;

		if (_gameMode == GameMode::PlayAsClient)
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

	_gameMode == GameMode::PlayAsClient ? SubscribeAsClient() : SubscribeAsHost();
}

void BonusSpawner::SubscribeAsHost()
{
	_events->AddListener("TickUpdate", _name, [this](const double /*deltaTime*/)
	{
		this->Update();
	});
}

void BonusSpawner::SubscribeAsClient()
{
	_events->AddListener(
			"ClientReceived_BonusSpawn", _name,
			[this](const FPoint pos, const BonusType type, const buuid& uuid)
			{
				const auto size = static_cast<float>(_bonusSize);
				const unsigned int color = RandUtils::GetRandNumber(_distRandColor);
				const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = size, .h = size};
				SpawnBonus(rect, color, type, uuid);
			});
}

void BonusSpawner::Unsubscribe() const
{
	_events->RemoveListener("GameModeChangedTo", _name);

	_gameMode == GameMode::PlayAsClient ? UnsubscribeAsClient() : UnsubscribeAsHost();
}

void BonusSpawner::UnsubscribeAsHost() const
{
	_events->RemoveListener("TickUpdate", _name);
}

void BonusSpawner::UnsubscribeAsClient() const
{
	_events->RemoveListener("ClientReceived_BonusSpawn", _name);
}

void BonusSpawner::Update()
{
	if (TimeUtils::IsCooldownFinish(_lastTimeSpawn, _cooldownBonusSpawn))//TODO: extract to timer manager
	{
		const auto size = static_cast<float>(_bonusSize);
		const auto x = static_cast<float>(RandUtils::GetRandNumber(_distSpawnPosX));
		const auto y = static_cast<float>(RandUtils::GetRandNumber(_distSpawnPosY));
		const ObjRectangle rect{.x = x, .y = y, .w = size, .h = size};
		const bool isFreeSpawnSpot = !std::ranges::any_of(*_allObjects, [&rect](const std::shared_ptr<BaseObj>& object)
		{
			return ColliderUtils::IsCollide(rect, object->GetRect());
		});

		if (isFreeSpawnSpot)
		{
			SpawnRandomBonus(rect);
		}
	}
}

void BonusSpawner::SpawnBonus(const ObjRectangle rect, const unsigned int color, const BonusType type, buuid uuid)
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
		case BonusType::Timer:
			bonus = std::make_shared<BonusTimer>(rect, _events, lifetime, color, uuid, _gameMode, duration);
			break;
		case BonusType::Helmet:
			bonus = std::make_shared<BonusHelmet>(rect, _events, lifetime, color, uuid, _gameMode, duration);
			break;
		case BonusType::Grenade:
			bonus = std::make_shared<BonusGrenade>(rect, _events, lifetime, color, uuid, _gameMode);
			break;
		case BonusType::Tank:
			bonus = std::make_shared<BonusTank>(rect, _events, lifetime, color, uuid, _gameMode);
			break;
		case BonusType::Star:
			bonus = std::make_shared<BonusStar>(rect, _events, lifetime, color, uuid, _gameMode);
			break;
		case BonusType::Shovel:
			bonus = std::make_shared<BonusShovel>(rect, _events, lifetime, color, uuid, _gameMode, duration);
			break;
		case BonusType::Caliber:
			bonus = std::make_shared<BonusCaliber>(rect, _events, lifetime, color, uuid, _gameMode);
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
	const unsigned int color = RandUtils::GetRandNumber(_distRandColor);//TODO: remove color from bonus
	const auto bonusType = static_cast<BonusType>(RandUtils::GetRandNumber(_distSpawnType));
	SpawnBonus(rect, color, bonusType);
}
