#pragma once

#include "enums/RespawnCount.h"
#include <boost/uuid/uuid.hpp>

enum class TankType : char8_t;
enum class GameMode : char8_t;
class EventSystem;

class RespawnManager final
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

	std::string _name{"RespawnManager"};

	std::shared_ptr<EventSystem> _events{nullptr};

	// TODO: use std::atomic when multithreading is used
	std::vector<unsigned short> _respawnCount{20u, 3u, 3u};

	struct SpawnSlot
	{
		buuid uuid{};
		bool isAvailable{};
	};

	GameMode _gameMode{};
	unsigned short _enemiesSpawnCount{};
	unsigned short _enemiesDeathCount{};
	unsigned short _playersSpawnCount{};
	unsigned short _playersDeathCount{};

	void OnBonusTank(const std::string& author);
	void OnClientRespawn(TankType type);

	void Subscribe();
	void SubscribeAsClient();

	void Unsubscribe() const;
	void UnsubscribeAsClient() const;

	void SetEnemyNeedRespawn();
	void SetPlayerNeedRespawn();

	void ResetRespawnStat();
	void ResetSpawn();
	void OnGameModeChange();

	static std::string RespawnCountEnumToString(RespawnCount type);
	void ChangeRespawnCount(int delta, RespawnCount type);
	void TriggerLastPlayersLife();

	void OnTankSpawn(const buuid& uuid);
	void EnemyDied(bool isAvailable);
	void PlayerDied(bool isAvailable);
	void OnTankDied(const buuid& uuid);

public:
	std::vector<SpawnSlot> _slots{};

	explicit RespawnManager(const std::shared_ptr<EventSystem>& events);

	~RespawnManager();
};
