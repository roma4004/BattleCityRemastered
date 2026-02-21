#pragma once

#include "utils/Timer.h"

class EventSystem;

class BonusEffectManager
{
	using milliseconds = std::chrono::milliseconds;

	Timer _timerEnemy{};
	Timer _timerPlayer{};
	Timer _shovelPlayer{};
	std::vector<Timer> _helmetSlots{};
	std::string _name{};

	std::shared_ptr<EventSystem> _events{nullptr};

public:
	explicit BonusEffectManager(const std::shared_ptr<EventSystem>& events);

	~BonusEffectManager();

	void Subscribe();
	void Unsubscribe() const;

	void Reset();

	void OnBonusStatusChange(const std::string& event, const std::string& id, bool value) const;
	void OnBonusShovelPickup(const std::string& fraction, milliseconds effectDuration);
	[[nodiscard]] static size_t TankNameToId(const std::string& name);

	void TickUpdate(double deltaTime);

	[[nodiscard]] Timer GetTimerEnemy() const;
	[[nodiscard]] Timer GetTimerPlayer() const;

	[[nodiscard]] Timer GetHelmet(size_t id) const;
};
