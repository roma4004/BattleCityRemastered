#pragma once

#include "enums/RespawnResource.h"
#include <boost/uuid/uuid.hpp>

enum class TankType : char8_t;
enum class GameMode : char8_t;
class EventSystem;

class RespawnResourceManager final
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

	std::string _name{"RespawnResourceManager"};

	std::shared_ptr<EventSystem> _events{nullptr};

	// TODO: use std::atomic when multithreading is used
	std::vector<int> _respawnResource{20, 3, 3};

	struct SpawnSlot
	{
		buuid uuid{};
		bool isAvailable{false};
	};

	GameMode _gameMode{};

	void OnBonusGrenade(const std::string& author, const std::string& fraction);
	void OnBonusTank(const std::string& author, const std::string& fraction);
	void OnClientRespawn(TankType type);

	void Subscribe();
	void SubscribeAsClient();

	void Unsubscribe() const;
	void UnsubscribeAsClient() const;

	void SetEnemyNeedRespawn();
	void SetPlayerNeedRespawn();

	void ResetRespawnStat();
	void ResetSpawn();

	static std::string RespawnResourceEnumToString(RespawnResource type);
	void ChangeRespawnResource(int delta, RespawnResource type);

	void OnTankSpawn(const buuid& uuid);
	void OnTankDied(const buuid& uuid);

public:
	std::vector<SpawnSlot> _slots{};

	explicit RespawnResourceManager(const std::shared_ptr<EventSystem>& events);

	~RespawnResourceManager();

	// NOTE: for unit tests only:
	[[nodiscard]] int GetEnemyRespawnResource() const;
	[[nodiscard]] int GetPlayerOneRespawnResource() const;
	[[nodiscard]] int GetPlayerTwoRespawnResource() const;

	void SetSlotNeedRespawn(int slotIndex);
};
