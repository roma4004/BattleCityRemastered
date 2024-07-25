#include "../../headers/Bonuses/BonusStar.h"

BonusStar::BonusStar(const ObjRectangle& rect, int* windowBuffer, const UPoint windowSize,
                     std::shared_ptr<EventSystem> events, const int durationMs, const int lifeTimeMs, const int color)
	: Bonus{rect, windowBuffer, windowSize, std::move(events), durationMs, lifeTimeMs, color}
{
	_name = "BonusStar " + std::to_string(reinterpret_cast<unsigned long long>(reinterpret_cast<void**>(this)));
	Subscribe();
}

BonusStar::~BonusStar()
{
	Unsubscribe();
}

void BonusStar::Subscribe()
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

void BonusStar::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener<const float>("TickUpdate", _name);

	_events->RemoveListener("Draw", _name);
}

void BonusStar::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>("BonusStar", author, fraction);
}

void BonusStar::PickUpBonus(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>("BonusStar", author, fraction);
}
