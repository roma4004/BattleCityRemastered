#include "../../headers/Bonuses/BonusTimer.h"

BonusTimer::BonusTimer(const Rectangle& rect, int* windowBuffer, const UPoint windowSize,
                       std::shared_ptr<EventSystem> events, const int durationSec, const int lifeTimeSec,
                       const int color)
	: Bonus{rect, windowBuffer, windowSize, std::move(events), durationSec, lifeTimeSec, color}
{
	_name = "BonusTimer " + std::to_string(reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));
	Subscribe();
}

BonusTimer::~BonusTimer()
{
	Unsubscribe();
};

void BonusTimer::Subscribe()
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

void BonusTimer::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener<const float>("TickUpdate", _name);

	_events->RemoveListener("Draw", _name);
}

void BonusTimer::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>("BonusTimer", author, fraction);
}

void BonusTimer::PickUpBonus(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&, int>("BonusTimer", author, fraction, _bonusDurationSec);
	SetIsAlive(false);
}
