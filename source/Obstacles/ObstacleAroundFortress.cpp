#include "../../headers/obstacles/ObstacleAroundFortress.h"

#include "../../headers/obstacles/Water.h"
#include "../../headers/utils/ColliderUtils.h"
#include "../../headers/utils/TimeUtils.h"

#include <string>

ObstacleAroundFortress::ObstacleAroundFortress(const Rectangle& rect, int* windowBuffer, UPoint windowSize,
                                               const std::shared_ptr<EventSystem>& events,
                                               std::vector<std::shared_ptr<BaseObj>>* allObjects)
	: BaseObj{{rect.x, rect.y, rect.w - 1, rect.h - 1}, 0x924b00, 15},
	  _rect{rect},
	  _windowSize{windowSize},
	  _windowBuffer{windowBuffer},
	  _events{events},
	  _allObjects{allObjects},
	  _brick{std::make_unique<Brick>(rect, windowBuffer, windowSize, events)}

{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);
	_name = "BrickAroundFortress " + std::to_string(
			        reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));
	Subscribe();
}

ObstacleAroundFortress::~ObstacleAroundFortress()
{
	Unsubscribe();
}

void ObstacleAroundFortress::BonusShovelSwitch()
{
	bool isFreeSpawnSpot = true;
	for (const std::shared_ptr<BaseObj>& object: *_allObjects)
	{
		if (dynamic_cast<ObstacleAroundFortress*>(object.get()))
		{
			continue;
		}
		if (dynamic_cast<Water*>(object.get()))
		{
			continue;
		}

		if (ColliderUtils::IsCollide(_rect, object->GetShape()))
		{
			isFreeSpawnSpot = false;
			break;
		}
	}

	if (isFreeSpawnSpot)
	{
		if (_isBrick)
		{
			_brick = nullptr;
			_iron = std::make_unique<Iron>(_rect, _windowBuffer, _windowSize, _events);
			SetColor(0xaaaaaa);
			BaseObj::SetIsPassable(false);
			BaseObj::SetIsDestructible(false);
			BaseObj::SetIsPenetrable(false);
		}
		else
		{
			_iron = nullptr;
			_brick = std::make_unique<Brick>(_rect, _windowBuffer, _windowSize, _events);
			SetColor(0x924b00);
			BaseObj::SetIsPassable(false);
			BaseObj::SetIsDestructible(true);
			BaseObj::SetIsPenetrable(false);
		}
	}

	_isBrick = !_isBrick;
}

void ObstacleAroundFortress::TakeDamage(const int damage)
{
	if (_isBrick)
	{
		_brick->TakeDamage(damage);
		if (_brick->GetHealth() < 1)
		{
			BaseObj::SetIsPassable(true);
			BaseObj::SetIsDestructible(false);
			BaseObj::SetIsPenetrable(true);
		}
	}
	else
	{
		_iron->TakeDamage(damage);
		if (_iron->GetHealth() < 1)
		{
			BaseObj::SetIsPassable(true);
			BaseObj::SetIsDestructible(false);
			BaseObj::SetIsPenetrable(true);
		}
	}
}

void ObstacleAroundFortress::Hide()
{
	if (_isBrick)
	{
		_brick->TakeDamage(_brick->GetHealth());
	}
	else
	{
		_iron->TakeDamage(_iron->GetHealth());
	}

	BaseObj::SetIsPassable(true);
	BaseObj::SetIsDestructible(false);
	BaseObj::SetIsPenetrable(true);
}

void ObstacleAroundFortress::Subscribe()
{
	if (_events == nullptr)
	{
		return;
	}

	_events->AddListener<const float>("TickUpdate", _name, [this](const float deltaTime)
	{
		this->TickUpdate(deltaTime);
	});

	_events->AddListener("Draw", _name, [this]() { this->Draw(); });

	_events->AddListener<const std::string&, const std::string&, int>(
			"BonusShovel",
			_name,
			[this](const std::string& author, const std::string& fraction, const int bonusDurationTimeMs)
			{
				if (fraction == "PlayerTeam")
				{
					if (_isActiveShovel)
					{
						_cooldownShovelMs += bonusDurationTimeMs;
						return;
					}

					_isActiveShovel = true;
					this->BonusShovelSwitch();
					_cooldownShovelMs = bonusDurationTimeMs;
				}
				else if (fraction == "EnemyTeam")
				{
					if (!this->_isBrick)
					{
						this->BonusShovelSwitch();
						_isActiveShovel = false;
					}

					this->Hide();
				}

				_activateTimeShovel = std::chrono::system_clock::now();
			});
}

void ObstacleAroundFortress::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener<const float>("TickUpdate", _name);
	_events->RemoveListener("Draw", _name);
	_events->RemoveListener<const std::string&, const std::string&, int>("BonusShovel", _name);
}

void ObstacleAroundFortress::Draw() const {}

void ObstacleAroundFortress::TickUpdate(const float /*deltaTime*/)
{
	// bonus disable timer
	if (_isActiveShovel && TimeUtils::IsCooldownFinish(_activateTimeShovel, _cooldownShovelMs))
	{
		_isActiveShovel = false;
		_cooldownShovelMs = 0;
		this->BonusShovelSwitch();
	}
}

void ObstacleAroundFortress::SendDamageStatistics(const std::string& author, const std::string& fraction) {}
