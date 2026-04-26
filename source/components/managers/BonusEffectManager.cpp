#include "components/managers/BonusEffectManager.h"
#include "components/EventSystem.h"
#include "entities/pawns/Tank.h"
#include "utils/TimeUtils.h"

BonusEffectManager::BonusEffectManager(const std::shared_ptr<EventSystem>& events)
	: _name{"BonusEffectManager"}
	, _events{events}
{
	Reset();

	Subscribe();
}

BonusEffectManager::~BonusEffectManager()
{
	Unsubscribe();
}

void BonusEffectManager::Subscribe()
{
	_events->AddListener("Reset", _name, [this]() { this->Reset(); });

	_events->AddListener("TickUpdate", _name, [this](const double deltaTime) { this->TickUpdate(deltaTime); });

	_events->AddListener("BonusTimer", _name, [this](const std::string& fraction, const milliseconds effectDuration)
	{
		this->OnTimerBonus(fraction, effectDuration);
	});
	_events->AddListener("BonusHelmet", _name, [this](const std::string& name, const milliseconds effectDuration)
	{
		this->OnHelmetBonus(name, effectDuration);
	});

	_events->AddListener("BonusShovel", _name, [this](const std::string& fraction, const milliseconds effectDuration)
	{
		this->OnBonusShovelPickup(fraction, effectDuration);
	});
	_events->AddListener("SpawnEnabled", _name, [this](std::shared_ptr<Tank> tank)
	{
		if (!tank)
		{
			return;
		}

		const std::string tankName{tank->GetName()};
		const std::string tankFraction{tank->GetFraction()};
		this->ApplyBonusEffectsTo(tankName, tankFraction); //NOTE: continue effects after respawn
	});
}

void BonusEffectManager::Unsubscribe() const { _events->RemoveAllListeners(_name); }

void BonusEffectManager::Reset()
{
	_timerEnemy = {};
	_timerPlayer = {};
	_shovelPlayer = {};
	_helmetSlots = {{}, {}, {}, {}, {}, {}};
}

void BonusEffectManager::ApplyBonusEffectsTo(const std::string& tankName, const std::string& tankFraction)
{
	if (tankFraction == "EnemyTeam")
	{
		_events->EmitEvent("BonusTimer_EffectOnOff", _timerEnemy.isActive, tankName);
	}
	else if (tankFraction == "PlayerTeam")
	{
		_events->EmitEvent("BonusTimer_EffectOnOff", _timerPlayer.isActive, tankName);
	}

	const size_t typeId = TankNameToId(tankName);
	_events->EmitEvent("BonusHelmet_EffectOnOff", _helmetSlots[typeId].isActive, tankName);
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

void BonusEffectManager::OnHelmetBonus(const std::string& name, const milliseconds effectDuration)
{
	if (const size_t id{TankNameToId(name)};
		id < _helmetSlots.size())
	{
		StartTimer(_helmetSlots[id], "Helmet", name, effectDuration);
	}
}

void BonusEffectManager::OnBonusStatusChange(const std::string& event, const std::string& id, const bool value) const
{
	_events->EmitEvent("Bonus" + event + "StatusChange", id, value);

	//TODO: move replication to bonusEffectManager from tank subscription
	// if (_gameMode == GameMode::PlayAsHost)
	// {
	// 	_events->EmitEvent("ServerSend_OnBonusHelmet", _name, isActive);
	// }
}

void BonusEffectManager::StartTimer(Timer& timer, const std::string& event, const std::string& id,
									const milliseconds effectDuration) const
{
	timer = {effectDuration, std::chrono::system_clock::now()};
	OnBonusStatusChange(event, id, timer.isActive);
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
			//TODO: change enemy1 and other to tankType
			FinishTimer(_helmetSlots[i], "Helmet", TankIdToName(i));
		}
	}

	if (_shovelPlayer.isActive && TimeUtils::IsCooldownFinish(_shovelPlayer.activateTime, _shovelPlayer.cooldown))
	{
		_shovelPlayer.isActive = false;
		_events->EmitEvent("BonusShovelOnCooldownEnd");
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
		if (_shovelPlayer.isActive)
		{
			_shovelPlayer.cooldown += effectDuration;
		}
		else
		{
			_shovelPlayer.cooldown = effectDuration;
			_shovelPlayer.isActive = true;
			_events->EmitEvent("BonusShovelOnPlayerPickup");
		}
	}
	else if (fraction == "EnemyTeam")
	{
		_shovelPlayer.isActive = false;
		_events->EmitEvent("BonusShovelOnEnemyPickup");
	}

	_shovelPlayer.activateTime = std::chrono::system_clock::now();
}

std::string BonusEffectManager::TankIdToName(const size_t id)
{
	switch (id)
	{
		case 0:
			return "Enemy1";//TODO: change enemy1 and other to tankType
		case 1:
			return "Enemy2";
		case 2:
			return "Enemy3";
		case 3:
			return "Enemy4";
		case 5:
			return "Player1";
		case 6:
			return "Player2";
		default:
			return "None";
	}
}

size_t BonusEffectManager::TankNameToId(const std::string_view& name)
{
	std::string nameStr{name};

	return TankNameToId(nameStr);
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
