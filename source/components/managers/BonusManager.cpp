#include "components/managers/BonusManager.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/SpawnEvents.h"
#include "components/events/TimingEvents.h"
#include "entities/bonuses/Bonus.h"
#include "enums/Faction.h"
#include "utils/Uuid.h"
#include <algorithm>
#include <variant>

BonusManager::BonusManager(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig)
	: _events{events}
	, _gameConfig{gameConfig}
{
	Subscribe();
}

void BonusManager::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &BonusManager::OnGameReset));
	_subs.push_back(_events->AddListener(this, &BonusManager::OnTickUpdate));
	_subs.push_back(_events->AddListener(this, &BonusManager::OnTimerBonus));
	_subs.push_back(_events->AddListener(this, &BonusManager::OnBonusHelmetPickup));
	_subs.push_back(_events->AddListener(this, &BonusManager::OnBonusShovelPickup));
	_subs.push_back(_events->AddListener(this, &BonusManager::OnBonusCreated));
	//NOTE: continue effects after respawn
	_subs.push_back(_events->AddListener(this, &BonusManager::ApplyBonusEffectsOnSpawnTo));
}

void BonusManager::OnGameReset(const GameResetEvent&) { Reset(); }

void BonusManager::Reset()
{
	_spawnedBonuses.clear();
	_activeEffects.clear();
}

void BonusManager::OnTickUpdate(const TickUpdateEvent&)
{
	ExpireBonuses();
	ExpireEffects();
}

void BonusManager::OnBonusCreated(const BonusCreatedEvent& event)
{
	_spawnedBonuses.emplace_back(SpawnedBonus{.bonus = event.bonus,
											  .lifeTime = Timer{_gameConfig.bonusLifeTimeCooldown}});
}

//NOTE: one clock for every bonus on the field, instead of a tick subscription inside each of them
void BonusManager::ExpireBonuses()
{
	for (const auto& [bonus, lifeTime]: _spawnedBonuses)
	{
		if (const std::shared_ptr<Bonus> alive = bonus.lock();
			alive && lifeTime.IsCooldownFinish())
		{
			alive->Expire();
		}
	}

	std::erase_if(_spawnedBonuses, [](const SpawnedBonus& spawned)
	{
		const std::shared_ptr<Bonus> bonus = spawned.bonus.lock();

		return bonus == nullptr || !bonus->GetIsAlive();
	});
}

void BonusManager::ExpireEffects()
{
	for (const auto& [type, target, timer]: _activeEffects)
	{
		if (timer.IsCooldownFinish())
		{
			EmitEffectStatus(type, target, false);
		}
	}

	std::erase_if(_activeEffects, [](const ActiveEffect& effect) { return effect.timer.IsCooldownFinish(); });
}

std::vector<BonusManager::ActiveEffect>::iterator BonusManager::FindEffect(const BonusType type,
																		   const EffectTarget& target)
{
	return std::ranges::find_if(_activeEffects, [type, &target](const ActiveEffect& effect)
	{
		return effect.type == type && effect.target == target;
	});
}

bool BonusManager::IsEffectActive(const BonusType type, const EffectTarget& target) const
{
	return std::ranges::any_of(_activeEffects, [type, &target](const ActiveEffect& effect)
	{
		return effect.type == type && effect.target == target;
	});
}

void BonusManager::StartEffect(const BonusType type, EffectTarget target, const milliseconds duration)
{
	if (const auto it = FindEffect(type, target);
		it != _activeEffects.end())
	{
		//NOTE: picking the same bonus up again buys more of the same effect, it does not restart it
		it->timer.cooldown += duration;

		return;
	}

	_activeEffects.emplace_back(ActiveEffect{.type = type, .target = std::move(target), .timer = Timer{duration}});
	EmitEffectStatus(type, _activeEffects.back().target, true);
}

void BonusManager::FinishEffect(const BonusType type, const EffectTarget& target)
{
	if (const auto it = FindEffect(type, target);
		it != _activeEffects.end())
	{
		_activeEffects.erase(it);
	}

	EmitEffectStatus(type, target, false);
}

void BonusManager::EmitEffectStatus(const BonusType type, const EffectTarget& target, const bool isActive) const
{
	//NOTE: the type is what says which alternative the target holds, so every get here is the one
	//StartEffect put in
	switch (type)
	{
		case BonusType::Timer:
			_events->EmitEvent(Key(std::get<Faction>(target)), BonusTimerStatusChangeEvent{.isActive = isActive});
			break;
		case BonusType::Helmet:
			_events->EmitEvent(Key(std::get<std::string>(target)), BonusHelmetStatusChangeEvent{.isActive = isActive});
			break;
		case BonusType::Shovel:
			_events->EmitEvent(
					BonusShovelStatusChangeEvent{.faction = std::get<Faction>(target), .isActive = isActive});
			break;
		default:
			break;//NOTE: every other bonus lands in one step, it has nothing left running
	}
}

void BonusManager::OnBonusHelmetPickup(const BonusHelmetPickupEvent& event)
{
	StartEffect(BonusType::Helmet, event.author, kEffectDuration);
}

void BonusManager::OnTimerBonus(const BonusTimerPickupEvent& event)
{
	StartEffect(BonusType::Timer, event.target, kEffectDuration);
}

void BonusManager::OnBonusShovelPickup(const BonusShovelPickupEvent& event)
{
	Faction faction = event.faction;
	if (faction == Faction::PlayerTeam)
	{
		StartEffect(BonusType::Shovel, faction, kEffectDuration);
	}
	else if (faction == Faction::EnemyTeam)//NOTE: enemy pickup should disable player shovel instantly
	{
		FinishEffect(BonusType::Shovel, faction);
	}
}

void BonusManager::ApplyBonusEffectsOnSpawnTo(const BonusReApplyEvent& event)
{
	const bool isFrozen = IsEffectActive(BonusType::Timer, event.faction);
	_events->EmitEvent(Key(event.uuid), BonusTimerReApplyOnSpawnEvent{.isEnabled = isFrozen});

	StartEffect(BonusType::Helmet, event.name, kRespawnHelmetDuration);
}
