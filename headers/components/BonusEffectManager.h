#pragma once

#include "../bonuses/BonusStatus.h"

class EventSystem;

class BonusEffectManager
{
	using milliseconds = std::chrono::milliseconds;

	BonusStatus _timerEnemy;
	BonusStatus _timerPlayer;
	BonusStatus _shovelPlayer;
	std::vector<BonusStatus> _helmetSlots;
	std::string _name;

	std::shared_ptr<EventSystem> _events;

public:
	explicit BonusEffectManager(std::shared_ptr<EventSystem> events);

	~BonusEffectManager();

	void Subscribe();
	void Unsubscribe() const;

	void Reset();

	void OnBonusStatusChange(const std::string& event, const std::string& id, bool value) const;
	void OnBonusShovelPickup(const std::string& fraction, milliseconds effectDuration);

	void TickUpdate(float);

	[[nodiscard]] BonusStatus GetTimerEnemy() const;
	[[nodiscard]] BonusStatus GetTimerPlayer() const;

	[[nodiscard]] BonusStatus GetHelmet(int id) const;
};
