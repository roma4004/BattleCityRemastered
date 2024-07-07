#include "../headers/BonusSystem.h"
#include "../headers/BonusGrenade.h"
#include "../headers/BonusHelmet.h"
#include "../headers/BonusTimer.h"
#include "../headers/ColliderCheck.h"

#include <chrono>
#include <limits>
#include <memory>

class BaseObj;
class EventSystem;

BonusSystem::BonusSystem(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                         int* windowBuffer, const UPoint windowSize, const int sideBarWidth, const int bonusSize)
	: _name{"BonusSystem"},
	  _events{std::move(events)},
	  _windowSize{windowSize},
	  _bonusSize{bonusSize},
	  _windowBuffer{windowBuffer},
	  _allObjects{allObjects},
	  _distSpawnPosY{0, static_cast<int>(_windowSize.y) - bonusSize},
	  _distSpawnPosX{0, static_cast<int>(_windowSize.x) - sideBarWidth},
	  _distSpawnType{0, 2},
	  _distRandColor{0, std::numeric_limits<int>::max()}
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
			"BonusTimerEnable",
			_name,
			[this](const std::string& author, const std::string& fraction, const int bonusDurationTime)
			{
				if (fraction == "EnemyTeam")
				{
					_playerTimerActivateTime = std::chrono::system_clock::now();
					_isActivePlayerTimer = true;
					_cooldownPlayerTimer = bonusDurationTime;
				}
				else if (fraction == "PlayerTeam")
				{
					_enemyTimerActivateTime = std::chrono::system_clock::now();
					_isActiveEnemyTimer = true;
					_cooldownEnemyTimer = bonusDurationTime;
				}
			});
	_events->AddListener<const std::string&, const std::string&, int>(
			"BonusHelmetEnable",
			_name,
			[this](const std::string& author, const std::string& fraction, const int bonusDurationTime)
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

	_events->RemoveListener<const std::string&, const std::string&, int>("BonusTimerEnable", _name);
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
		const float size = static_cast<float>(_bonusSize);
		Rectangle rect{static_cast<float>(_distSpawnPosX(_gen)), static_cast<float>(_distSpawnPosY(_gen)), size, size};
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
			const int color = _distRandColor(_gen);
			const int bonusId = _distSpawnType(_gen);
			constexpr int bonusLifetime = 10;
			constexpr int bonusDurationTime = 10;
			if (bonusId == 0)
			{
				_allObjects->emplace_back(std::make_shared<BonusTimer>(rect, _windowBuffer, _windowSize, _events,
				                                                       bonusDurationTime, bonusLifetime, color));
			}
			else if (bonusId == 1)
			{
				_allObjects->emplace_back(std::make_shared<BonusHelmet>(rect, _windowBuffer, _windowSize, _events,
				                                                        bonusDurationTime, bonusLifetime, color));
			}
			else if (bonusId == 2)
			{
				_allObjects->emplace_back(std::make_shared<BonusGrenade>(rect, _windowBuffer, _windowSize, _events,
				                                                         bonusDurationTime, bonusLifetime, color));
			}

			_lastTimeBonusSpawn = std::chrono::system_clock::now();
		}
	}

	if (_isActiveEnemyTimer && IsCooldownFinish(_enemyTimerActivateTime, _cooldownEnemyTimer))
	{
		_events->EmitEvent<const std::string&, const std::string&>("BonusTimerDisable", "Enemy1", "EnemyTeam");
		_isActiveEnemyTimer = false;
	}

	if (_isActivePlayerTimer && IsCooldownFinish(_playerTimerActivateTime, _cooldownPlayerTimer))
	{
		_events->EmitEvent<const std::string&, const std::string&>("BonusTimerDisable", "PlayerOne", "PlayerTeam");
		_isActivePlayerTimer = false;
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
