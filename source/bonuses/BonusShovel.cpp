#include "../../headers/Bonuses/BonusShovel.h"
#include "../../headers/bonuses/BonusTypeId.h"

BonusShovel::BonusShovel(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                         const int durationMs, const int lifeTimeMs, const int color, const int id,
                         const GameMode gameMode)
	: Bonus{rect, std::move(window), std::move(events), durationMs, lifeTimeMs, color, "BonusShovel", id, gameMode}
{
	_events->EmitEvent<const std::string&, const FPoint, const BonusTypeId, const int>(
			"ServerSend_BonusSpawn", _name, FPoint{rect.x, rect.y}, Shovel, _id);
}

BonusShovel::~BonusShovel() = default;

void BonusShovel::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>("BonusShovel", author, fraction);
}

void BonusShovel::PickUpBonus(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&, int>("BonusShovel", author, fraction, _bonusDurationMs);
}
