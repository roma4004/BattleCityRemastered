#include "../../headers/bonuses/BonusHelmet.h"
#include "../../headers/bonuses/BonusTypeId.h"

BonusHelmet::BonusHelmet(const ObjRectangle& rect, std::shared_ptr<int[]> windowBuffer, UPoint windowSize,
                         std::shared_ptr<EventSystem> events, const int durationMs, const int lifeTimeMs,
                         const int color, const int id)
	: Bonus{rect, std::move(windowBuffer), std::move(windowSize), std::move(events), durationMs, lifeTimeMs, color, id}
{
	_name = "BonusHelmet";
	Subscribe();

	_events->EmitEvent<const std::string&, const FPoint, const BonusTypeId, const int>(
			"ServerSend_BonusSpawn", _name, FPoint{rect.x, rect.y}, BonusTypeId::Helmet, _id);
}

BonusHelmet::~BonusHelmet()
{
	Unsubscribe();

	_events->EmitEvent<const int>("ServerSend_BonusDeSpawn", _id);
}

void BonusHelmet::Subscribe()
{
	if (_events == nullptr)
	{
		return;
	}

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

void BonusHelmet::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener<const float>("TickUpdate", _name + std::to_string(_id));

	_events->RemoveListener<const int>("ClientReceived_BonusDeSpawn", _name);

	_events->RemoveListener("Draw", _name + std::to_string(_id));
}

void BonusHelmet::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>("BonusHelmet", author, fraction);
}

void BonusHelmet::PickUpBonus(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&, int>("BonusHelmet", author, fraction, _bonusDurationMs);
}
