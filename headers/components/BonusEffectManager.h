#pragma once

#include "../bonuses/BonusStatus.h"

class EventSystem;

class BonusEffectManager
{
	BonusStatus _timerEnemy;
	BonusStatus _timerPlayer;
	std::string _name;

	std::shared_ptr<EventSystem> _events;
	std::vector<BonusStatus> _helmetSlots;

public:
	explicit BonusEffectManager(std::shared_ptr<EventSystem> events);

	~BonusEffectManager();

	void Subscribe();
	void Unsubscribe() const;

	void OnBonusStatusChange(const std::string& event, const std::string& id, bool value) const;

	void TickUpdate(float);

	[[nodiscard]] BonusStatus GetTimerEnemy() const;
	[[nodiscard]] BonusStatus GetTimerPlayer() const;

	[[nodiscard]] BonusStatus GetHelmet(int id) const;
};
