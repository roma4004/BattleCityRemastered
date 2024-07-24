#include "../../headers/bonuses/BonusHelmet.h"

BonusHelmet::BonusHelmet(const ObjRectangle& rect, int* windowBuffer, const UPoint windowSize,
                         std::shared_ptr<EventSystem> events, const int durationMs, const int lifeTimeMs,
                         const int color)
	: Bonus{rect, windowBuffer, windowSize, std::move(events), durationMs, lifeTimeMs, color}
{
	_name = "BonusHelmet " + std::to_string(reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));
	Subscribe();
}

BonusHelmet::~BonusHelmet()
{
	Unsubscribe();
};

void BonusHelmet::Subscribe()
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

void BonusHelmet::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener<const float>("TickUpdate", _name);

	_events->RemoveListener("Draw", _name);
}

void BonusHelmet::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>("BonusHelmet", author, fraction);
}

void BonusHelmet::PickUpBonus(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&, int>("BonusHelmet", author, fraction, _bonusDurationMs);
}