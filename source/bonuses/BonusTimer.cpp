#include "../../headers/Bonuses/BonusTimer.h"
#include "../../headers/bonuses/BonusTypeId.h"

BonusTimer::BonusTimer(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                       const int durationMs, const int lifeTimeMs, const int color, const int id)
	: Bonus{rect, std::move(window), std::move(events), durationMs, lifeTimeMs, color, "BonusTimer", id}
{
	Subscribe();

	_events->EmitEvent<const std::string&, const FPoint, const BonusTypeId, const int>(
			"ServerSend_BonusSpawn", _name, FPoint{rect.x, rect.y}, Timer, _id);
}

BonusTimer::~BonusTimer()
{
	Unsubscribe();

	_events->EmitEvent<const int>("ServerSend_BonusDeSpawn", _id);//TODO: move to pick up moment in tank move beh
}

void BonusTimer::Subscribe()
{
	_events->AddListener<const float>("TickUpdate", _name + std::to_string(_id), [this](const float deltaTime)
	{
		this->TickUpdate(deltaTime);
	});

	_events->AddListener<const int>("ClientReceived_BonusDeSpawn", _name, [this](const int id)
	{
		if (id == this->_id)
		{
			this->SetIsAlive(false);
		}
	});

	_events->AddListener("Draw", _name + std::to_string(_id), [this]() { this->Draw(); });
}

void BonusTimer::Unsubscribe() const
{
	_events->RemoveListener<const float>("TickUpdate", _name + std::to_string(_id));

	_events->RemoveListener<const int>("ClientReceived_BonusDeSpawn", _name);

	_events->RemoveListener("Draw", _name + std::to_string(_id));
}

void BonusTimer::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>("BonusTimer", author, fraction);
}

void BonusTimer::PickUpBonus(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&, int>("BonusTimer", author, fraction, _bonusDurationMs);
}
