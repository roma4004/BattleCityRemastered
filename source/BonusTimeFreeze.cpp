#include "../headers/BonusTeamFreeze.h"

BonusTeamFreeze::BonusTeamFreeze(const Rectangle& rect, int* windowBuffer, const UPoint windowSize,
                                 std::shared_ptr<EventSystem> events, const int bonusDurationTime, const int lifeTimeSec)
	: Bonus{rect, windowBuffer, windowSize, std::move(events), bonusDurationTime, lifeTimeSec, 0x00ff00}
{
	_name = "BonusTeamFreeze " + std::to_string(reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));
	Subscribe();
}

BonusTeamFreeze::~BonusTeamFreeze()
{
	Unsubscribe();
};

void BonusTeamFreeze::Subscribe()
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
}

void BonusTeamFreeze::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener<const float>("TickUpdate", _name);

	_events->RemoveListener("Draw", _name);
}

void BonusTeamFreeze::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>("BonusTeamFreeze", author, fraction);
}

void BonusTeamFreeze::PickUpBonus(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&, int>("BonusTeamFreezeEnable", author, fraction,
	                                                                _bonusDurationSec);
	SetIsAlive(false);
}
