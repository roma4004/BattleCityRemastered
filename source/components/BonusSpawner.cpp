#include "components/BonusSpawner.h"
#include "Point.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/TimingEvents.h"
#include "components/SpawnEvents.h"
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
						   std::vector<std::shared_ptr<BaseObj>>* allObjects, GameConfig& gameConfig)
	: _events{events}
	, _allObjects{allObjects}
	, _distSpawnPosY{0, static_cast<int>(gameConfig.windowSize.y) - gameConfig.bonusSize}
	, _distSpawnPosX{0, static_cast<int>(gameConfig.windowSize.x - gameConfig.sideBarWidth) - gameConfig.bonusSize}
	, _distSpawnType{static_cast<int>(BonusType::None) + 1, static_cast<int>(BonusType::lastId) - 1}
	, _gameConfig{gameConfig}
	, _spawnTimer{std::chrono::seconds{60}, std::chrono::system_clock::now()}
{
	Subscribe();
}

void BonusSpawner::Subscribe()
{
	_subs.push_back(_events->AddListener(_name, [this](const GameResetEvent&) { this->Reset(); }));
	_subs.push_back(_events->AddListener(_name, [this](const GameModeChangedToEvent& event)
	{
		this->_gameMode = event.mode;

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
	}));

	_subs.push_back(_events->AddListener(_name, [this](const WindowSizeChangedToEvent& event)
	{
		const UPoint& newSize = event.newSize;
		_distSpawnPosY = std::uniform_int_distribution<>{
				0,
				static_cast<int>(newSize.y) - _gameConfig.bonusSize};
		_distSpawnPosX = std::uniform_int_distribution<>{
				0,
				static_cast<int>(newSize.x - _gameConfig.sideBarWidth) - _gameConfig.bonusSize};
	}));

	_gameMode == GameMode::PlayAsClient ? SubscribeAsClient() : SubscribeAsHost();
}

void BonusSpawner::SubscribeAsHost()
{
	_hostSub = _events->AddListener(_name, [this](const TickUpdateEvent&) { this->Update(); });
}

void BonusSpawner::SubscribeAsClient()
{
	_clientSub = _events->AddListener(_name, [this](const ClientReceivedBonusSpawnEvent& event)
	{
		const auto size = static_cast<float>(_gameConfig.bonusSize);
		const ObjRectangle rect{.x = event.pos.x, .y = event.pos.y, .w = size, .h = size};
		SpawnBonus(rect, event.type, event.uuid);
	});
}

void BonusSpawner::UnsubscribeAsHost() { _hostSub = EventSubscription{}; }

void BonusSpawner::UnsubscribeAsClient() { _clientSub = EventSubscription{}; }

void BonusSpawner::Update()
{
	if (_spawnTimer.IsCooldownFinish())
	{
		const auto size = static_cast<float>(_gameConfig.bonusSize);
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
			_spawnTimer.Reset();
		}
	}
}

void BonusSpawner::SpawnBonus(const ObjRectangle rect, const BonusType type, buuid uuid)
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
			bonus = std::make_shared<BonusTimer>(rect, _events, lifetime, uuid, _gameMode, duration);
			break;
		case BonusType::Helmet:
			bonus = std::make_shared<BonusHelmet>(rect, _events, lifetime, uuid, _gameMode, duration);
			break;
		case BonusType::Grenade:
			bonus = std::make_shared<BonusGrenade>(rect, _events, lifetime, uuid, _gameMode);
			break;
		case BonusType::Tank:
			bonus = std::make_shared<BonusTank>(rect, _events, lifetime, uuid, _gameMode);
			break;
		case BonusType::Star:
			bonus = std::make_shared<BonusStar>(rect, _events, lifetime, uuid, _gameMode);
			break;
		case BonusType::Shovel:
			bonus = std::make_shared<BonusShovel>(rect, _events, lifetime, uuid, _gameMode, duration);
			break;
		case BonusType::Caliber:
			bonus = std::make_shared<BonusCaliber>(rect, _events, lifetime, uuid, _gameMode);
			break;
		default:
			break;
	}

	if (bonus)
	{
		_events->EmitEvent(AddToSpawnQueueEvent{.obj = std::shared_ptr<BaseObj>{bonus}});
	}
}

void BonusSpawner::SpawnRandomBonus(const ObjRectangle rect)
{
	const auto bonusType = static_cast<BonusType>(RandUtils::GetRandNumber(_distSpawnType));
	SpawnBonus(rect, bonusType);
}

void BonusSpawner::Reset()
{
	_spawnTimer.Reset();
}
