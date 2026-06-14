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
	std::vector<unsigned short> _respawnCount{20, 3, 3};

	struct SpawnSlot
	{
		buuid uuid{};
		bool isAvailable{false};
	};

	GameMode _gameMode{};
	unsigned short _enemiesSpawnCount{0};
	unsigned short _enemiesDeathCount{0};
	unsigned short _playersSpawnCount{0};
	unsigned short _playersDeathCount{0};

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

	// NOTE: for unit tests only:
	[[nodiscard]] int GetEnemyRespawnCount() const;
	[[nodiscard]] int GetPlayerOneRespawnCount() const;
	[[nodiscard]] int GetPlayerTwoRespawnCount() const;

	void SetSlotNeedRespawn(int slotIndex);
};
