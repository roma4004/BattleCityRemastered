#include "components/managers/BonusManager.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/SpawnEvents.h"
#include "components/events/TimingEvents.h"
#include "entities/bonuses/Bonus.h"
#include "enums/Faction.h"
#include "utils/TimeUtils.h"
#include "utils/Uuid.h"
#include <algorithm>
#include <ranges>
#include <variant>
#include <vector>

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

//NOTE: one reading of the clock for both passes - asking twice let a deadline fall between them,
//and that effect vanished unannounced, leaving its helmet on
void BonusManager::ExpireEffects()
{
	const auto isExpired = [now = TimeUtils::Now()](const ActiveEffect& effect)
	{
		return effect.timer.IsCooldownFinish(now);
	};

	for (const auto& [type, target, timer]: _activeEffects | std::views::filter(isExpired))
	{
		EmitEffectStatus(type, target, false);
	}

	std::erase_if(_activeEffects, isExpired);
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

void BonusManager::StartEffect(const BonusType type, const EffectTarget target, const milliseconds duration)
{
	if (const auto it = FindEffect(type, target);
		it != _activeEffects.end())
	{
		//NOTE: picking the same bonus up again buys more of the same effect, it does not restart it
		it->timer.cooldown += duration;

		return;
	}

	_activeEffects.emplace_back(ActiveEffect{.type = type, .target = target, .timer = Timer{duration}});
	EmitEffectStatus(type, target, true);
}

void BonusManager::FinishEffect(const BonusType type, const EffectTarget& target)
{
	const auto it = FindEffect(type, target);
	if (it == _activeEffects.end())
	{
		return;
	}

	_activeEffects.erase(it);
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
			_events->EmitEvent(Key(std::get<Author>(target)), BonusHelmetStatusChangeEvent{.isActive = isActive});
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
	if (event.faction == Faction::PlayerTeam)
	{
		StartEffect(BonusType::Shovel, event.faction, kEffectDuration);

		return;
	}

	//NOTE: two things in one pickup - the player's steel walls end early, and the fortress comes down
	FinishEffect(BonusType::Shovel, Faction::PlayerTeam);
	_events->EmitEvent(BonusShovelStatusChangeEvent{.faction = Faction::EnemyTeam, .isActive = false});
}

void BonusManager::ApplyBonusEffectsOnSpawnTo(const BonusReApplyEvent& event)
{
	//NOTE: effect timing is the host's - the client is told when one starts and when it ends, and
	//keeps no clock of its own to go stale
	if (!_gameConfig.IsAuthority())
	{
		return;
	}

	const bool isFrozen = IsEffectActive(BonusType::Timer, FactionOf(event.author));
	_events->EmitEvent(Key(event.uuid), BonusTimerReApplyOnSpawnEvent{.isEnabled = isFrozen});

	StartEffect(BonusType::Helmet, event.author, kRespawnHelmetDuration);
}
