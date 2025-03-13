#include "../../headers/bonuses/BonusHelmet.h"
#include "../../headers/bonuses/BonusTypeId.h"

BonusHelmet::BonusHelmet(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                         const std::chrono::milliseconds duration, const std::chrono::milliseconds lifeTime,
                         const int color, const int id, const GameMode gameMode)
	: Bonus{rect, std::move(window), std::move(events), duration, lifeTime, color, "BonusHelmet", id, gameMode}
{
	_events->EmitEvent<const std::string&, const FPoint, const BonusTypeId, const int>(
			"ServerSend_BonusSpawn", _name, FPoint{rect.x, rect.y}, Helmet, _id);
}

BonusHelmet::~BonusHelmet() = default;

void BonusHelmet::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>("BonusHelmet", author, fraction);
}

void BonusHelmet::PickUpBonus(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&, std::chrono::milliseconds>(
			"BonusHelmet", author, fraction, _bonusDuration);
}
