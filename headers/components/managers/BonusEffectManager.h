#pragma once
#include "components/EventSystem.h"
#include "utils/Timer.h"
#include <boost/uuid/uuid.hpp>

enum class GameMode : char8_t;
class EventSystem;
struct GameResetEvent;
struct TickUpdateEvent;
struct GameModeChangedToEvent;
struct BonusTimerPickupEvent;
struct BonusHelmetPickupEvent;
struct BonusShovelPickupEvent;
struct BonusReApplyEvent;

class BonusEffectManager
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

	Timer _timerEnemy{};
	Timer _timerPlayer{};
	Timer _shovelPlayer{};
	std::vector<Timer> _helmetSlots{};
	std::vector<std::string> _helmetSlotsTankNames{};
	GameMode _gameMode{};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

public:
	explicit BonusEffectManager(const std::shared_ptr<EventSystem>& events);

	~BonusEffectManager() = default;

	void Subscribe();
	void OnGameReset(const GameResetEvent&);
	void OnTickUpdate(const TickUpdateEvent& event);
	void OnBonusHelmetPickup(const BonusHelmetPickupEvent& event);

	void Reset();

	void ApplyBonusEffectsOnSpawnTo(const BonusReApplyEvent& event);
	void OnTimerBonus(const BonusTimerPickupEvent& event);
	void OnHelmetBonusPickup(const std::string& author, milliseconds effectDuration);
	void OnBonusStatusChange(const std::string& event, const std::string& id, bool isActive) const;
	void StartTimer(Timer& timer, const std::string& event, const std::string& id, milliseconds effectDuration) const;
	void FinishTimer(Timer& timer, const std::string& event, const std::string& id) const;
	void OnBonusShovelPickup(const BonusShovelPickupEvent& event);
	void OnGameModeChangedTo(const GameModeChangedToEvent& event);
	static size_t TankNameToId(const std::string& name);

	void TickUpdate(double deltaTime);

	[[nodiscard]] Timer GetTimerEnemy() const;
	[[nodiscard]] Timer GetTimerPlayer() const;

	[[nodiscard]] Timer GetHelmet(size_t id) const;
};
