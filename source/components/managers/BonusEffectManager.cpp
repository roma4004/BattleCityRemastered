#include "components/managers/BonusEffectManager.h"
#include "components/EventSystem.h"
#include "utils/TimeUtils.h"

BonusEffectManager::BonusEffectManager(std::shared_ptr<EventSystem> events)
	: _name{"BonusEffectManager"}, _events{std::move(events)}
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

	_events->AddListener("TickUpdate", _name, [this](const float deltaTime)
	{
		this->TickUpdate(deltaTime);
	});

	_events->AddListener("TimerActive", _name, [this](const std::string& fraction, const milliseconds effectDuration)
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

	_events->AddListener("HelmetActive", _name, [this](const std::string& name, const milliseconds effectDuration)
	{
		if (name == "Enemy1")//TODO: change enemy1 and other to tankType
		{
			_helmetSlots[0] = {effectDuration, std::chrono::system_clock::now()};
			OnBonusStatusChange("Helmet", "Enemy1", _helmetSlots[0].isActive);
		}
		else if (name == "Enemy2")
		{
			_helmetSlots[1] = {effectDuration, std::chrono::system_clock::now()};
			OnBonusStatusChange("Helmet", "Enemy2", _helmetSlots[1].isActive);
		}
		else if (name == "Enemy3")
		{
			_helmetSlots[2] = {effectDuration, std::chrono::system_clock::now()};
			OnBonusStatusChange("Helmet", "Enemy3", _helmetSlots[2].isActive);
		}
		else if (name == "Enemy4")
		{
			_helmetSlots[3] = {effectDuration, std::chrono::system_clock::now()};
			OnBonusStatusChange("Helmet", "Enemy4", _helmetSlots[3].isActive);
		}
		else if (name == "Player1")
		{
			_helmetSlots[4] = {effectDuration, std::chrono::system_clock::now()};
			OnBonusStatusChange("Helmet", "Player1", _helmetSlots[4].isActive);
		}
		else if (name == "Player2")
		{
			_helmetSlots[5] = {effectDuration, std::chrono::system_clock::now()};
			OnBonusStatusChange("Helmet", "Player2", _helmetSlots[5].isActive);
		}
	});


	_events->AddListener(//TODO: remove duration for bonuses
			"BonusShovel", _name,
			[this](const std::string& /*author*/, const std::string& fraction, const milliseconds effectDuration)
			{
				this->OnBonusShovelPickup(fraction, effectDuration);
			});
}

void BonusEffectManager::Unsubscribe() const
{
	_events->RemoveListener("Reset", _name);
	_events->RemoveListener("TickUpdate", _name);
	_events->RemoveListener("TimerActive", _name);
	_events->RemoveListener("HelmetActive", _name);
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
}

void BonusEffectManager::TickUpdate(const float /*deltaTime*/)
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

Timer BonusEffectManager::GetHelmet(const int id) const//TODO: change to size_t
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
