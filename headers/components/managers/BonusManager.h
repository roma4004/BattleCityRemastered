#pragma once
#include "components/EventSystem.h"
#include "entities/EffectTarget.h"
#include "enums/Author.h"
#include "enums/BonusType.h"
#include "utils/Timer.h"
#include <chrono>
#include <memory>
#include <string>
#include <vector>

struct BonusCreatedEvent;
struct GameResetEvent;
struct TickUpdateEvent;
struct BonusTimerPickupEvent;
struct BonusHelmetPickupEvent;
struct BonusShovelPickupEvent;
struct BonusEffectTimeLeft;
struct BonusReApplyEvent;
struct WorldSnapshotRequestedEvent;
class Bonus;
class EventSystem;
class GameConfig;

class BonusManager final
{
	using milliseconds = std::chrono::milliseconds;

	static constexpr milliseconds kRespawnHelmetDuration{std::chrono::seconds{5}};

	struct SpawnedBonus
	{
		std::weak_ptr<Bonus> bonus{};
		Timer lifeTime{};
	};

	struct ActiveEffect
	{
		BonusType type{};
		EffectTarget target{};
		Timer timer{};
		//NOTE: who a free-for-all timer leaves running
		Author spared{};
	};

	std::vector<SpawnedBonus> _spawnedBonuses{};
	std::vector<ActiveEffect> _activeEffects{};

	std::shared_ptr<EventSystem> _events{nullptr};
	const GameConfig& _gameConfig;
	std::vector<EventSubscription> _subs{};

	void Subscribe();
	void OnGameReset(const GameResetEvent&);
	void OnTickUpdate(const TickUpdateEvent& event);
	void OnBonusCreated(const BonusCreatedEvent& event);
	void OnBonusHelmetPickup(const BonusHelmetPickupEvent& event);
	void OnTimerBonus(const BonusTimerPickupEvent& event);
	void OnBonusShovelPickup(const BonusShovelPickupEvent& event);
	void ApplyBonusEffectsOnSpawnTo(const BonusReApplyEvent& event);
	void OnWorldSnapshotRequested(const WorldSnapshotRequestedEvent& event) const;

	void ExpireBonuses();
	void ExpireEffects();
	void Reset();

	void StartEffect(BonusType type, EffectTarget target, milliseconds duration, Author spared = {});
	void FinishEffect(BonusType type, const EffectTarget& target);
	void EmitEffectStatus(const ActiveEffect& effect, bool isActive) const;
	[[nodiscard]] std::vector<BonusEffectTimeLeft> RunningEffects() const;
	[[nodiscard]] std::vector<ActiveEffect>::iterator FindEffect(BonusType type, const EffectTarget& target);

public:
	BonusManager(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig);
};
