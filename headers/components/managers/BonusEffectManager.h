#pragma once

#include "entities/pawns/Tank.h"
#include "utils/Timer.h"

class EventSystem;

class BonusEffectManager
{
	using milliseconds = std::chrono::milliseconds;

	Timer _timerEnemy{};
	Timer _timerPlayer{};
	Timer _shovelPlayer{};
	std::vector<Timer> _helmetSlots{};
	std::vector<std::string> _helmetSlotsTankNames{};
	std::string _name{};

	std::shared_ptr<EventSystem> _events{nullptr};

public:
	explicit BonusEffectManager(const std::shared_ptr<EventSystem>& events);

	~BonusEffectManager();

	void Subscribe();
	void Unsubscribe() const;

	void Reset();

	void ApplyBonusEffectsTo(const std::string& tankName, const std::string& tankFraction);
	void OnTimerBonus(const std::string& fraction, milliseconds effectDuration);
	void OnHelmetBonus(const std::string& name, milliseconds effectDuration);
	void OnBonusStatusChange(const std::string& event, const std::string& id, bool isActive) const;
	void StartTimer(Timer& timer, const std::string& event, const std::string& id, milliseconds effectDuration) const;
	void FinishTimer(Timer& timer, const std::string& event, const std::string& id) const;
	void OnBonusShovelPickup(const std::string& fraction, milliseconds effectDuration);
	static size_t TankNameToId(const std::string_view& name);

	void TickUpdate(double deltaTime);

	[[nodiscard]] Timer GetTimerEnemy() const;
	[[nodiscard]] Timer GetTimerPlayer() const;

	[[nodiscard]] Timer GetHelmet(size_t id) const;
};
