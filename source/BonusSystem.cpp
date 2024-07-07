#include "../headers/BonusSystem.h"

#include <chrono>
#include <memory>

class EventSystem;

BonusSystem::BonusSystem(std::shared_ptr<EventSystem> events)
	: _name{"BonusSystem"}, _events{std::move(events)}
{
	Subscribe();
}

BonusSystem::~BonusSystem()
{
	Unsubscribe();
};

void BonusSystem::Subscribe()
{
	if (_events == nullptr)
	{
		return;
	}

	_events->AddListener<const float>("TickUpdate", _name, [this](const float deltaTime)
	{
		this->TickUpdate(deltaTime);
	});

	_events->AddListener<const std::string&, const std::string&, int>(
			"BonusTeamFreezeEnable",
			_name,
			[this](const std::string& author, const std::string& fraction, int bonusDurationTime)
			{
				if (fraction == "EnemyTeam")
				{
					playerTeamFreezeActivateTime = std::chrono::system_clock::now();
					isActivePlayerTeamFreeze = true;
					cooldownPlayerTeamFreeze = bonusDurationTime;
				}
				else if (fraction == "PlayerTeam")
				{
					enemyTeamFreezeActivateTime = std::chrono::system_clock::now();
					isActiveEnemyTeamFreeze = true;
					cooldownEnemyTeamFreeze = bonusDurationTime;
				}
			});
	_events->AddListener<const std::string&, const std::string&, int>(
			"BonusHelmetEnable",
			_name,
			[this](const std::string& author, const std::string& fraction, int bonusDurationTime)
			{
				cooldownHelmet = bonusDurationTime;

				if (fraction == "EnemyTeam")
				{
					if (author == "Enemy1")
					{
						isActiveEnemyOneHelmet = true;
						enemyOneHelmetActivateTime = std::chrono::system_clock::now();
					}
					else if (author == "Enemy2")
					{
						isActiveEnemyTwoHelmet = true;
						enemyTwoHelmetActivateTime = std::chrono::system_clock::now();
					}
					else if (author == "Enemy3")
					{
						isActiveEnemyThreeHelmet = true;
						enemyThreeHelmetActivateTime = std::chrono::system_clock::now();
					}
					else if (author == "Enemy4")
					{
						isActiveEnemyFourHelmet = true;
						enemyFourHelmetActivateTime = std::chrono::system_clock::now();
					}
				}
				else if (fraction == "PlayerTeam")
				{
					if (author == "PlayerOne")
					{
						isActivePlayerOneHelmet = true;
						playerOneHelmetActivateTime = std::chrono::system_clock::now();
					}
					else if (author == "PlayerTwo")
					{
						isActivePlayerTwoHelmet = true;
						playerTwoHelmetActivateTime = std::chrono::system_clock::now();
					}
				}
			});
}

void BonusSystem::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener<const float>("TickUpdate", _name);

	_events->RemoveListener<const std::string&, const std::string&, int>("BonusTeamFreezeEnable", _name);
}

bool BonusSystem::IsCooldownFinish(const std::chrono::system_clock::time_point activateTime, const int cooldown)
{
	const auto activateTimeSec =
			std::chrono::duration_cast<std::chrono::seconds>(activateTime.time_since_epoch()).count();
	const auto currentSec =
			std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
			.count();

	if (currentSec - activateTimeSec >= cooldown)
	{
		return true;
	}

	return false;
}

void BonusSystem::TickUpdate(const float deltaTime)
{
	if (isActiveEnemyTeamFreeze && IsCooldownFinish(enemyTeamFreezeActivateTime, cooldownEnemyTeamFreeze))
	{
		_events->EmitEvent<const std::string&, const std::string&>("BonusTeamFreezeDisable", "Enemy1", "EnemyTeam");
		isActiveEnemyTeamFreeze = false;
	}

	if (isActivePlayerTeamFreeze && IsCooldownFinish(playerTeamFreezeActivateTime,
	                                                        cooldownPlayerTeamFreeze))
	{
		_events->EmitEvent<const std::string&, const std::string&>("BonusTeamFreezeDisable", "PlayerOne", "PlayerTeam");
		isActivePlayerTeamFreeze = false;
	}

	if (isActiveEnemyOneHelmet && IsCooldownFinish(enemyOneHelmetActivateTime, cooldownHelmet))
	{
		_events->EmitEvent<const std::string&, const std::string&>("BonusHelmetDisable", "Enemy1", "EnemyTeam");
		isActiveEnemyOneHelmet = false;
	}
	if (isActiveEnemyTwoHelmet && IsCooldownFinish(enemyTwoHelmetActivateTime, cooldownHelmet))
	{
		_events->EmitEvent<const std::string&, const std::string&>("BonusHelmetDisable", "Enemy2", "EnemyTeam");
		isActiveEnemyTwoHelmet = false;
	}
	if (isActiveEnemyThreeHelmet && IsCooldownFinish(enemyThreeHelmetActivateTime, cooldownHelmet))
	{
		_events->EmitEvent<const std::string&, const std::string&>("BonusHelmetDisable", "Enemy3", "EnemyTeam");
		isActiveEnemyThreeHelmet = false;
	}
	if (isActiveEnemyFourHelmet && IsCooldownFinish(enemyFourHelmetActivateTime, cooldownHelmet))
	{
		_events->EmitEvent<const std::string&, const std::string&>("BonusHelmetDisable", "Enemy4", "EnemyTeam");
		isActiveEnemyFourHelmet = false;
	}

	if (isActivePlayerOneHelmet && IsCooldownFinish(playerOneHelmetActivateTime, cooldownHelmet))
	{
		_events->EmitEvent<const std::string&, const std::string&>("BonusHelmetDisable", "PlayerOne", "PlayerTeam");
		isActivePlayerOneHelmet = false;
	}
	if (isActivePlayerTwoHelmet && IsCooldownFinish(playerTwoHelmetActivateTime, cooldownHelmet))
	{
		_events->EmitEvent<const std::string&, const std::string&>("BonusHelmetDisable", "PlayerTwo", "PlayerTeam");
		isActivePlayerTwoHelmet = false;
	}
}
