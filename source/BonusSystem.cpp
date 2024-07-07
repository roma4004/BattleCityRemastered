#include "../headers/BonusSystem.h"
#include "../headers/BonusHelmet.h"
#include "../headers/BonusTeamFreeze.h"
#include "../headers/ColliderCheck.h"

#include <chrono>
#include <memory>

class BaseObj;
class EventSystem;

BonusSystem::BonusSystem(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                         int* windowBuffer, const UPoint windowSize, const int sideBarWidth)
	: _name{"BonusSystem"},
	  _events{std::move(events)},
	  _windowSize{windowSize},
	  _windowBuffer{windowBuffer},
	  _allObjects{allObjects},
	  _distSpawnPosY{0, static_cast<int>(_windowSize.y)},
	  _distSpawnPosX{0, static_cast<int>(_windowSize.x) - sideBarWidth},
	  _distSpawnType{0, 1}
{
	std::random_device rd;
	_gen = std::mt19937(std::chrono::high_resolution_clock::now().time_since_epoch().count() + rd());

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
					_playerTeamFreezeActivateTime = std::chrono::system_clock::now();
					_isActivePlayerTeamFreeze = true;
					_cooldownPlayerTeamFreeze = bonusDurationTime;
				}
				else if (fraction == "PlayerTeam")
				{
					_enemyTeamFreezeActivateTime = std::chrono::system_clock::now();
					_isActiveEnemyTeamFreeze = true;
					_cooldownEnemyTeamFreeze = bonusDurationTime;
				}
			});
	_events->AddListener<const std::string&, const std::string&, int>(
			"BonusHelmetEnable",
			_name,
			[this](const std::string& author, const std::string& fraction, int bonusDurationTime)
			{
				_cooldownHelmet = bonusDurationTime;

				if (fraction == "EnemyTeam")
				{
					if (author == "Enemy1")
					{
						_isActiveEnemyOneHelmet = true;
						_enemyOneHelmetActivateTime = std::chrono::system_clock::now();
					}
					else if (author == "Enemy2")
					{
						_isActiveEnemyTwoHelmet = true;
						_enemyTwoHelmetActivateTime = std::chrono::system_clock::now();
					}
					else if (author == "Enemy3")
					{
						_isActiveEnemyThreeHelmet = true;
						_enemyThreeHelmetActivateTime = std::chrono::system_clock::now();
					}
					else if (author == "Enemy4")
					{
						_isActiveEnemyFourHelmet = true;
						_enemyFourHelmetActivateTime = std::chrono::system_clock::now();
					}
				}
				else if (fraction == "PlayerTeam")
				{
					if (author == "PlayerOne")
					{
						_isActivePlayerOneHelmet = true;
						_playerOneHelmetActivateTime = std::chrono::system_clock::now();
					}
					else if (author == "PlayerTwo")
					{
						_isActivePlayerTwoHelmet = true;
						_playerTwoHelmetActivateTime = std::chrono::system_clock::now();
					}
					else if (author == "CoopOneAI")
					{
						_isActiveCoopOneAIHelmet = true;
						_coopOneAIHelmetActivateTime = std::chrono::system_clock::now();
					}
					else if (author == "CoopTwoAI")
					{
						_isActiveCoopTwoAIHelmet = true;
						_coopTwoAIHelmetActivateTime = std::chrono::system_clock::now();
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
	if (IsCooldownFinish(_lastTimeBonusSpawn, _cooldownBonusSpawn))
	{
		bool isFreeSpawnSpot = true;
		Rectangle rect{static_cast<float>(_distSpawnPosX(_gen)), static_cast<float>(_distSpawnPosY(_gen)), 36.f, 36.f};
		for (const std::shared_ptr<BaseObj>& object: *_allObjects)
		{
			if (ColliderCheck::IsCollide(rect, object->GetShape()))
			{
				isFreeSpawnSpot = false;
				break;
			}
		}

		if (isFreeSpawnSpot)
		{
			const int bonusId = _distSpawnType(_gen);
			constexpr int bonusLifetime = 10;
			constexpr int bonusDurationTime = 10;
			if (bonusId == 0)
			{
				_allObjects->emplace_back(std::make_shared<BonusHelmet>(rect, _windowBuffer, _windowSize, _events,
				                                                        bonusDurationTime, bonusLifetime));
			}
			else if (bonusId == 1)
			{
				_allObjects->emplace_back(std::make_shared<BonusTeamFreeze>(rect, _windowBuffer, _windowSize, _events,
				                                                            bonusDurationTime, bonusLifetime));
			}

			_lastTimeBonusSpawn = std::chrono::system_clock::now();
		}
	}

	if (_isActiveEnemyTeamFreeze && IsCooldownFinish(_enemyTeamFreezeActivateTime, _cooldownEnemyTeamFreeze))
	{
		_events->EmitEvent<const std::string&, const std::string&>("BonusTeamFreezeDisable", "Enemy1", "EnemyTeam");
		_isActiveEnemyTeamFreeze = false;
	}

	if (_isActivePlayerTeamFreeze && IsCooldownFinish(_playerTeamFreezeActivateTime, _cooldownPlayerTeamFreeze))
	{
		_events->EmitEvent<const std::string&, const std::string&>("BonusTeamFreezeDisable", "PlayerOne", "PlayerTeam");
		_isActivePlayerTeamFreeze = false;
	}

	if (_isActiveEnemyOneHelmet && IsCooldownFinish(_enemyOneHelmetActivateTime, _cooldownHelmet))
	{
		_events->EmitEvent<const std::string&, const std::string&>("BonusHelmetDisable", "Enemy1", "EnemyTeam");
		_isActiveEnemyOneHelmet = false;
	}
	if (_isActiveEnemyTwoHelmet && IsCooldownFinish(_enemyTwoHelmetActivateTime, _cooldownHelmet))
	{
		_events->EmitEvent<const std::string&, const std::string&>("BonusHelmetDisable", "Enemy2", "EnemyTeam");
		_isActiveEnemyTwoHelmet = false;
	}
	if (_isActiveEnemyThreeHelmet && IsCooldownFinish(_enemyThreeHelmetActivateTime, _cooldownHelmet))
	{
		_events->EmitEvent<const std::string&, const std::string&>("BonusHelmetDisable", "Enemy3", "EnemyTeam");
		_isActiveEnemyThreeHelmet = false;
	}
	if (_isActiveEnemyFourHelmet && IsCooldownFinish(_enemyFourHelmetActivateTime, _cooldownHelmet))
	{
		_events->EmitEvent<const std::string&, const std::string&>("BonusHelmetDisable", "Enemy4", "EnemyTeam");
		_isActiveEnemyFourHelmet = false;
	}

	if (_isActivePlayerOneHelmet && IsCooldownFinish(_playerOneHelmetActivateTime, _cooldownHelmet))
	{
		_events->EmitEvent<const std::string&, const std::string&>("BonusHelmetDisable", "PlayerOne", "PlayerTeam");
		_isActivePlayerOneHelmet = false;
	}
	if (_isActivePlayerTwoHelmet && IsCooldownFinish(_playerTwoHelmetActivateTime, _cooldownHelmet))
	{
		_events->EmitEvent<const std::string&, const std::string&>("BonusHelmetDisable", "PlayerTwo", "PlayerTeam");
		_isActivePlayerTwoHelmet = false;
	}
	if (_isActiveCoopOneAIHelmet && IsCooldownFinish(_coopOneAIHelmetActivateTime, _cooldownHelmet))
	{
		_events->EmitEvent<const std::string&, const std::string&>("BonusHelmetDisable", "CoopOneAI", "PlayerTeam");
		_isActiveCoopOneAIHelmet = false;
	}
	if (_isActiveCoopTwoAIHelmet && IsCooldownFinish(_coopTwoAIHelmetActivateTime, _cooldownHelmet))
	{
		_events->EmitEvent<const std::string&, const std::string&>("BonusHelmetDisable", "CoopTwoAI", "PlayerTeam");
		_isActiveCoopTwoAIHelmet = false;
	}
}