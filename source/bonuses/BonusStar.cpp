#include "../../headers/Bonuses/BonusStar.h"
#include "../../headers/bonuses/BonusTypeId.h"

BonusStar::BonusStar(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                     const int durationMs, const int lifeTimeMs, const int color, const int id, const GameMode gameMode)
	: Bonus{rect, std::move(window), std::move(events), durationMs, lifeTimeMs, color, "BonusStar", id, gameMode}
{
	_events->EmitEvent<const std::string&, const FPoint, const BonusTypeId, const int>(
			"ServerSend_BonusSpawn", _name, FPoint{rect.x, rect.y}, Star, _id);
}

BonusStar::~BonusStar() = default;

void BonusStar::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>("BonusStar", author, fraction);
}

void BonusStar::PickUpBonus(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>("BonusStar", author, fraction);
}
