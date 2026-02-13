#include "components/managers/BonusEffectManager.h"
#include "components/EventSystem.h"
#include "utils/TimeUtils.h"

BonusEffectManager::BonusEffectManager(const std::shared_ptr<EventSystem>& events)
	: _name{"BonusEffectManager"}, _events{events}
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
	_events->AddListener("Reset", _name, [this]() { Reset(); });

	_events->AddListener("TickUpdate", _name, [this](const double deltaTime)
	{
		this->TickUpdate(deltaTime);
	});

	_events->AddListener("BonusTimer", _name, [this](const std::string& fraction, const milliseconds effectDuration)
	{
		if (fraction == "EnemyTeam")
		{
			_timerPlayer = {effectDuration, std::chrono::system_clock::now()};
			OnBonusStatusChange("Timer", "PlayerTeam", _timerPlayer.isActive);
		}
		else if (fraction == "PlayerTeam")
		{
			_timerEnemy = {effectDuration, std::chrono::system_clock::now()};
			OnBonusStatusChange("Timer", "EnemyTeam", _timerEnemy.isActive);
		}
	});

	_events->AddListener("BonusHelmet", _name, [this](const std::string& name, const milliseconds effectDuration)
	{
		if (const size_t id{TankNameToId(name)};
			id < _helmetSlots.size())
		{
			_helmetSlots[id] = {effectDuration, std::chrono::system_clock::now()};
			OnBonusStatusChange("Helmet", name, _helmetSlots[id].isActive);
		}
	});

	//TODO: remove duration for bonuses
	_events->AddListener("BonusShovel", _name, [this](const std::string& fraction, const milliseconds effectDuration)
	{
		this->OnBonusShovelPickup(fraction, effectDuration);
	});
}

void BonusEffectManager::Unsubscribe() const
{
	_events->RemoveListener("Reset", _name);
	_events->RemoveListener("TickUpdate", _name);
	_events->RemoveListener("BonusTimer", _name);
	_events->RemoveListener("BonusHelmet", _name);
	_events->RemoveListener("BonusShovel", _name);
}

void BonusEffectManager::Reset()
{
	_timerEnemy = {};
	_timerPlayer = {};
	_shovelPlayer = {};
	_helmetSlots = {{}, {}, {}, {}, {}, {}};
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

void BonusEffectManager::TickUpdate(const double /*deltaTime*/)
{
	if (_timerEnemy.isActive && TimeUtils::IsCooldownFinish(_timerEnemy.activateTime, _timerEnemy.cooldown))
	{
		_timerEnemy.isActive = false;
		OnBonusStatusChange("Timer", "EnemyTeam", _timerPlayer.isActive);
	}

	if (_timerPlayer.isActive && TimeUtils::IsCooldownFinish(_timerPlayer.activateTime, _timerPlayer.cooldown))
	{
		_timerPlayer.isActive = false;
		OnBonusStatusChange("Timer", "PlayerTeam", _timerPlayer.isActive);
	}

	for (size_t i = 0u; i < _helmetSlots.size(); ++i)
	{
		if (_helmetSlots[i].isActive && TimeUtils::IsCooldownFinish(_helmetSlots[i].activateTime,
		                                                            _helmetSlots[i].cooldown))
		{
			_helmetSlots[i].isActive = false;
			if (i == 0)
			{
				//TODO: change enemy1 and other to tankType
				OnBonusStatusChange("Helmet", "Enemy1", _helmetSlots[i].isActive);
			}
			else if (i == 1)
			{
				OnBonusStatusChange("Helmet", "Enemy2", _helmetSlots[i].isActive);
			}
			else if (i == 2)
			{
				OnBonusStatusChange("Helmet", "Enemy3", _helmetSlots[i].isActive);
			}
			else if (i == 3)
			{
				OnBonusStatusChange("Helmet", "Enemy4", _helmetSlots[i].isActive);
			}
			else if (i == 4)
			{
				OnBonusStatusChange("Helmet", "Player1", _helmetSlots[i].isActive);
			}
			else if (i == 5)
			{
				OnBonusStatusChange("Helmet", "Player2", _helmetSlots[i].isActive);
			}
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

size_t BonusEffectManager::TankNameToId(const std::string& name)
{
	if (name == "Enemy1")//TODO: change enemy1 and other to tankType
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

	if (name == "Player1")
	{
		return 4;
	}

	if (name == "Player2")
	{
		return 5;
	}

	return -1;
}
