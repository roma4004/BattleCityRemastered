#include "../../headers/components/BonusEffectManager.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/utils/TimeUtils.h"

BonusEffectManager::BonusEffectManager(std::shared_ptr<EventSystem> events)
	: _name{"BonusEffectManager"}, _events{std::move(events)}
{
	_helmetSlots = {{}, {}, {}, {}, {}, {},};

	Subscribe();
}

BonusEffectManager::~BonusEffectManager()
{
	Unsubscribe();
}

using milliseconds = std::chrono::milliseconds;

void BonusEffectManager::Subscribe()
{
	_events->AddListener<const std::string&, const milliseconds>(
			"TimerActive", _name,
			[this](const std::string& fraction, const milliseconds effectDuration)
			{
				if (fraction == "EnemyTeam")
				{
					_timerPlayer = {true, effectDuration, std::chrono::system_clock::now()};
					OnBonusStatusChange("Timer", "PlayerTeam", _timerPlayer.isActive);
				}
				else if (fraction == "PlayerTeam")
				{
					_timerEnemy = {true, effectDuration, std::chrono::system_clock::now()};
					OnBonusStatusChange("Timer", "EnemyTeam", _timerEnemy.isActive);
				}
			});

	_events->AddListener<const std::string&, const milliseconds>(
			"HelmetActive", _name,
			[this](const std::string& name, const milliseconds effectDuration)
			{
				if (name == "Enemy1")
				{
					_helmetSlots[0] = {true, effectDuration, std::chrono::system_clock::now()};
					OnBonusStatusChange("Helmet", "Enemy1", _helmetSlots[0].isActive);
				}
				else if (name == "Enemy2")
				{
					_helmetSlots[1] = {true, effectDuration, std::chrono::system_clock::now()};
					OnBonusStatusChange("Helmet", "Enemy2", _helmetSlots[1].isActive);
				}
				else if (name == "Enemy3")
				{
					_helmetSlots[2] = {true, effectDuration, std::chrono::system_clock::now()};
					OnBonusStatusChange("Helmet", "Enemy3", _helmetSlots[2].isActive);
				}
				else if (name == "Enemy4")
				{
					_helmetSlots[3] = {true, effectDuration, std::chrono::system_clock::now()};
					OnBonusStatusChange("Helmet", "Enemy4", _helmetSlots[3].isActive);
				}
				else if (name == "Player1")
				{
					_helmetSlots[4] = {true, effectDuration, std::chrono::system_clock::now()};
					OnBonusStatusChange("Helmet", "Player1", _helmetSlots[4].isActive);
				}
				else if (name == "Player2")
				{
					_helmetSlots[5] = {true, effectDuration, std::chrono::system_clock::now()};
					OnBonusStatusChange("Helmet", "Player2", _helmetSlots[5].isActive);
				}
			});

	_events->AddListener<const float>("TickUpdate", _name, [this](const float deltaTime)
	{
		this->TickUpdate(deltaTime);
	});
}

void BonusEffectManager::Unsubscribe() const
{
	_events->RemoveListener("TimerActive", _name);
	_events->RemoveListener("HelmetActive", _name);
}

void BonusEffectManager::OnBonusStatusChange(const std::string& event, const std::string& id, const bool value) const
{
	_events->EmitEvent<const std::string&, const bool>("Bonus" + event + "StatusChange", id, value);
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

	for (int i = 0; i < _helmetSlots.size(); ++i)
	{
		if (_helmetSlots[i].isActive && TimeUtils::IsCooldownFinish(_helmetSlots[i].activateTime,
		                                                            _helmetSlots[i].cooldown))
		{
			_helmetSlots[i].isActive = false;
			if (i == 0)
			{
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
}

BonusStatus BonusEffectManager::GetTimerEnemy() const { return _timerEnemy; }

BonusStatus BonusEffectManager::GetTimerPlayer() const { return _timerPlayer; }

BonusStatus BonusEffectManager::GetHelmet(const int id) const
{
	if (id < 0 || id >= _helmetSlots.size())
		return {};

	return _helmetSlots[id];
}
