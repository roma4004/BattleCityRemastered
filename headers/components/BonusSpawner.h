#pragma once

#include "components/EventSystem.h"
#include "utils/Timer.h"
#include <boost/uuid/uuid.hpp>
#include <random>
#include <vector>

enum class GameMode : char8_t;
enum class BonusType : char8_t;
struct UPoint;
struct ObjRectangle;
class BaseObj;
class EventSystem;
class GameConfig;
struct GameResetEvent;
struct GameModeChangedToEvent;
struct WindowSizeChangedToEvent;
struct TickUpdateEvent;
struct ClientInBonusSpawnEvent;

class BonusSpawner final
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

	std::shared_ptr<EventSystem> _events{nullptr};

	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};

	std::uniform_int_distribution<> _distSpawnPosY{};
	std::uniform_int_distribution<> _distSpawnPosX{};
	std::uniform_int_distribution<> _distSpawnType{};
	GameConfig& _gameConfig;

	Timer _spawnTimer;
	GameMode _gameMode{};

	std::vector<EventSubscription> _subs{};
	// Toggled at runtime on every GameModeChangedToEvent, independent of _subs's fixed
	// subscribe-once-at-construction lifetime - assigning a new EventSubscription here
	// auto-unsubscribes whatever was previously held.
	EventSubscription _hostSub{};
	EventSubscription _clientSub{};

	void Subscribe();
	void OnGameModeChangedTo(const GameModeChangedToEvent& event);
	void OnWindowSizeChangedTo(const WindowSizeChangedToEvent& event);
	void SubscribeAsHost();
	void SubscribeAsClient();
	void OnClientInBonusSpawn(const ClientInBonusSpawnEvent& event);

	void UnsubscribeAsHost();
	void UnsubscribeAsClient();

	void Update(const TickUpdateEvent&);
	void Reset(const GameResetEvent&);

public:
	BonusSpawner(const std::shared_ptr<EventSystem>& events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
				 GameConfig& gameConfig);

	~BonusSpawner() = default;

	void SpawnRandomBonus(ObjRectangle rect);

	void SpawnBonus(ObjRectangle rect, BonusType type, buuid uuid = {});//NOTE: for unit tests
};
