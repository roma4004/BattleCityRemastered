#include "components/managers/BonusEffectManager.h"
#include "utils/Uuid.h"
#include "components/EventSystem.h"
#include "components/events/SpawnEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/TimingEvents.h"
#include "utils/TimeUtils.h"

BonusEffectManager::BonusEffectManager(const std::shared_ptr<EventSystem>& events)
	: _events{events}
{
	Reset();

	Subscribe();
}

void BonusEffectManager::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &BonusEffectManager::OnGameReset));
	_subs.push_back(_events->AddListener(this, &BonusEffectManager::OnTickUpdate));
	_subs.push_back(_events->AddListener(this, &BonusEffectManager::OnTimerBonus));
	_subs.push_back(_events->AddListener(this, &BonusEffectManager::OnBonusHelmetPickup));
	_subs.push_back(_events->AddListener(this, &BonusEffectManager::OnBonusShovelPickup));
	//NOTE: continue effects after respawn
	_subs.push_back(_events->AddListener(this, &BonusEffectManager::ApplyBonusEffectsOnSpawnTo));
}

void BonusEffectManager::OnGameReset(const GameResetEvent&) { Reset(); }

void BonusEffectManager::OnTickUpdate(const TickUpdateEvent& event) { TickUpdate(event.deltaTime); }

void BonusEffectManager::OnBonusHelmetPickup(const BonusHelmetPickupEvent& event)
{
	OnHelmetBonusPickup(event.author, event.effectDuration);
}

void BonusEffectManager::Reset()
{
	_timerEnemy = {};
	_timerPlayer = {};
	_shovelPlayer = {};
	_helmetSlots = {{}, {}, {}, {}, {}, {}};
	_helmetSlotsTankNames = {{}, {}, {}, {}, {}, {}};
}

void BonusEffectManager::ApplyBonusEffectsOnSpawnTo(const BonusReApplyEvent& event)
{
	const bool isActive = event.fraction == "EnemyTeam" ? _timerEnemy.isActive : _timerPlayer.isActive;
	_events->EmitEvent(Key(event.uuid), BonusTimerReApplyOnSpawnEvent{.isEnabled = isActive});

	constexpr milliseconds effectDuration{std::chrono::seconds{5}};
	OnHelmetBonusPickup(event.name, effectDuration);
}

void BonusEffectManager::OnTimerBonus(const BonusTimerPickupEvent& event)
{
	const auto& [fraction, effectDuration] = event;
	if (fraction == "EnemyTeam")
	{
		StartTimer(_timerPlayer, "Timer", "PlayerTeam", effectDuration);
	}
	else if (fraction == "PlayerTeam")
	{
		StartTimer(_timerEnemy, "Timer", "EnemyTeam", effectDuration);
	}
}

void BonusEffectManager::OnHelmetBonusPickup(const std::string& author, const milliseconds effectDuration)
{
	if (const size_t timerId{TankNameToId(author)};
		timerId < _helmetSlots.size())
	{
		_helmetSlotsTankNames[timerId] = author;
		StartTimer(_helmetSlots[timerId], "Helmet", author, effectDuration);
	}
}

void BonusEffectManager::OnBonusStatusChange(const std::string& event, const std::string& id, const bool isActive) const
{
	if (event == "Timer")
	{
		_events->EmitEvent(BonusTimerStatusChangeEvent{.fraction = id, .isActive = isActive});
	}
	else if (event == "Helmet")
	{
		_events->EmitEvent(BonusHelmetStatusChangeEvent{.name = id, .isActive = isActive});
	}
	else if (event == "Shovel")
	{
		_events->EmitEvent(BonusShovelStatusChangeEvent{.fraction = id, .isActive = isActive});
	}
}

void BonusEffectManager::StartTimer(Timer& timer, const std::string& event, const std::string& id,
									const milliseconds effectDuration) const
{
	if (timer.isActive == false)
	{
		timer.Reset(effectDuration);
		OnBonusStatusChange(event, id, timer.isActive);
	}
	else
	{
		timer.cooldown += effectDuration;
	}
}

void BonusEffectManager::FinishTimer(Timer& timer, const std::string& event, const std::string& id) const
{
	timer.isActive = false;
	OnBonusStatusChange(event, id, timer.isActive);
}

void BonusEffectManager::TickUpdate(const double /*deltaTime*/)
{
	if (_timerEnemy.isActive && TimeUtils::IsCooldownFinish(_timerEnemy.activateTime, _timerEnemy.cooldown))
	{
		FinishTimer(_timerEnemy, "Timer", "EnemyTeam");
	}

	if (_timerPlayer.isActive && TimeUtils::IsCooldownFinish(_timerPlayer.activateTime, _timerPlayer.cooldown))
	{
		FinishTimer(_timerPlayer, "Timer", "PlayerTeam");
	}

	for (size_t i = 0u; i < _helmetSlots.size(); ++i)
	{
		if (_helmetSlots[i].isActive && TimeUtils::IsCooldownFinish(_helmetSlots[i].activateTime,
																	_helmetSlots[i].cooldown))
		{
			FinishTimer(_helmetSlots[i], "Helmet", _helmetSlotsTankNames[i]);
		}
	}

	if (_shovelPlayer.isActive && TimeUtils::IsCooldownFinish(_shovelPlayer.activateTime, _shovelPlayer.cooldown))
	{
		FinishTimer(_shovelPlayer, "Shovel", "PlayerTeam");
	}
}

Timer BonusEffectManager::GetTimerEnemy() const { return _timerEnemy; }

Timer BonusEffectManager::GetTimerPlayer() const { return _timerPlayer; }

Timer BonusEffectManager::GetHelmet(const size_t id) const
{
	if (id >= _helmetSlots.size())
	{
		return {};
	}

	return _helmetSlots[id];
}

void BonusEffectManager::OnBonusShovelPickup(const BonusShovelPickupEvent& event)
{
	const auto& [fraction, effectDuration] = event;
	if (fraction == "PlayerTeam")
	{
		StartTimer(_shovelPlayer, "Shovel", fraction, effectDuration);
	}
	else if (fraction == "EnemyTeam")//NOTE: enemy pickup should disable player shovel instantly
	{
		FinishTimer(_shovelPlayer, "Shovel", fraction);
	}
}

size_t BonusEffectManager::TankNameToId(const std::string& name)
{
	if (name == "Enemy1")
	{
		return 0;
	}

	if (name == "Enemy2")
	{
		return 1;
	}

	if (name == "Enemy3")
	{
		return 2;
	}

	if (name == "Enemy4")
	{
		return 3;
	}

	if (name == "Player1" || name == "CoopBot1")
	{
		return 4;
	}

	if (name == "Player2" || name == "CoopBot2")
	{
		return 5;
	}

	return static_cast<size_t>(-1);
}

