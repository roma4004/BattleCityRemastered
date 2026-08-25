#pragma once
#include "components/EventSystem.h"
#include "enums/BonusType.h"
#include "utils/Timer.h"
#include <string>
#include <variant>
#include <vector>

enum class Faction : char8_t;

class Bonus;
class EventSystem;
class GameConfig;
struct BonusCreatedEvent;
struct GameResetEvent;
struct TickUpdateEvent;
struct BonusTimerPickupEvent;
struct BonusHelmetPickupEvent;
struct BonusShovelPickupEvent;
struct BonusReApplyEvent;

class BonusManager
{
	using milliseconds = std::chrono::milliseconds;

	//NOTE: one bonus pickup is worth this much of its effect - a super one simply lands twice
	static constexpr milliseconds kEffectDuration{std::chrono::seconds{15}};
	//NOTE: the shorter grace a tank gets for free when it respawns
	static constexpr milliseconds kRespawnHelmetDuration{std::chrono::seconds{5}};

	//NOTE: the field owns the bonus, this only watches the clock on it - a weak handle so a bonus that
	//was picked up or shot simply drops out
	struct SpawnedBonus
	{
		std::weak_ptr<Bonus> bonus{};
		Timer lifeTime{};
	};

	//NOTE: an effect outlives the bonus that started it, so it is keyed by what it is on - a tank name
	//for the helmet, a faction for the rest. The BonusType next to it says which of the two this is
	using EffectTarget = std::variant<std::string, Faction>;

	struct ActiveEffect
	{
		BonusType type{};
		EffectTarget target{};
		Timer timer{};
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

	void ExpireBonuses();
	void ExpireEffects();
	void Reset();

	void StartEffect(BonusType type, EffectTarget target, milliseconds duration);
	void FinishEffect(BonusType type, const EffectTarget& target);
	void EmitEffectStatus(BonusType type, const EffectTarget& target, bool isActive) const;
	[[nodiscard]] bool IsEffectActive(BonusType type, const EffectTarget& target) const;
	[[nodiscard]] std::vector<ActiveEffect>::iterator FindEffect(BonusType type, const EffectTarget& target);

public:
	BonusManager(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig);

	~BonusManager() = default;
};
