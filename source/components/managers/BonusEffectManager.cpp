#include "components/managers/BonusEffectManager.h"
#include "components/EventSystem.h"
#include "components/SpawnEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/TimingEvents.h"
#include "enums/GameMode.h"
#include "utils/TimeUtils.h"

BonusEffectManager::BonusEffectManager(const std::shared_ptr<EventSystem>& events)
	: _name{"BonusEffectManager"}
	, _events{events}
{
	Reset();

	Subscribe();
}

void BonusEffectManager::Subscribe()
{
	_subs.push_back(_events->AddListener(_name, [this](const GameResetEvent&) { this->Reset(); }));

	_subs.push_back(_events->AddListener(_name, [this](const TickUpdateEvent& event)
	{
		this->TickUpdate(event.deltaTime);
	}));
	_subs.push_back(_events->AddListener(_name, [this](const GameModeChangedToEvent& event)
	{
		this->OnGameModeChangedTo(event.mode);
	}));

	_subs.push_back(_events->AddListener(_name, [this](const BonusTimerPickupEvent& event)
	{
		this->OnTimerBonus(event.fraction, event.effectDuration);
	}));
	_subs.push_back(_events->AddListener(_name, [this](const BonusHelmetPickupEvent& event)
	{
		this->OnHelmetBonusPickup(event.author, event.effectDuration);
	}));
	_subs.push_back(_events->AddListener(_name, [this](const BonusShovelPickupEvent& event)
	{
		this->OnBonusShovelPickup(event.fraction, event.effectDuration);
	}));

	_subs.push_back(_events->AddListener(_name, [this](const BonusEffectReApplyEvent& event)
	{
		this->OnSpawnEnabled(event.name, event.fraction);
		//TODO: refactor to enabled by uuid instead of name and fraction
	}));
}

void BonusEffectManager::Reset()
{
	_timerEnemy = {};
	_timerPlayer = {};
	_shovelPlayer = {};
	_helmetSlots = {{}, {}, {}, {}, {}, {}};
	_helmetSlotsTankNames = {{}, {}, {}, {}, {}, {}};
}

void BonusEffectManager::ApplyBonusEffectsOnSpawnTo(const std::string& tankName, const std::string& tankFraction)
{
	const bool isActive = tankFraction == "EnemyTeam" ? _timerEnemy.isActive : _timerPlayer.isActive;
	_events->EmitEvent(BonusTimerReApplyOnSpawnEvent{.isEnabled = isActive, .name = tankName});

	constexpr milliseconds effectDuration{std::chrono::seconds{5}};
	OnHelmetBonusPickup(tankName, effectDuration);
}

void BonusEffectManager::OnTimerBonus(const std::string& fraction, const milliseconds effectDuration)
{
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
		timer.cooldown = effectDuration;
		timer.activateTime = std::chrono::system_clock::now();
		timer.isActive = true;
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
	if (id < 0 || id >= _helmetSlots.size())
		return {};

	return _helmetSlots[id];
}

void BonusEffectManager::OnBonusShovelPickup(const std::string& fraction, const milliseconds effectDuration)
{
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

void BonusEffectManager::OnSpawnEnabled(const std::string& name, const std::string& fraction)
{
	ApplyBonusEffectsOnSpawnTo(name, fraction);//NOTE: continue effects after respawn
}

void BonusEffectManager::OnGameModeChangedTo(const GameMode newGameMode)
{
	this->_gameMode = newGameMode;
}
