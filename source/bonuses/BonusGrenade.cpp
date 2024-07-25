#include "../../headers/bonuses/BonusGrenade.h"

BonusGrenade::BonusGrenade(const ObjRectangle& rect, int* windowBuffer, const UPoint windowSize,
                           std::shared_ptr<EventSystem> events, const int durationMs, const int lifeTimeMs,
                           const int color)
	: Bonus{rect, windowBuffer, windowSize, std::move(events), durationMs, lifeTimeMs, color}
{
	_name = "BonusGrenade " + std::to_string(reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));
	Subscribe();
}

BonusGrenade::~BonusGrenade()
{
	Unsubscribe();
}

void BonusGrenade::Subscribe()
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

void BonusGrenade::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener<const float>("TickUpdate", _name);

	_events->RemoveListener("Draw", _name);
}

void BonusGrenade::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>("BonusGrenade", author, fraction);
}

void BonusGrenade::PickUpBonus(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>("BonusGrenade", author, fraction);
}
