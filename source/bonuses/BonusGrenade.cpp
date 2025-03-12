#include "../../headers/bonuses/BonusGrenade.h"
#include "../../headers/bonuses/BonusTypeId.h"

BonusGrenade::BonusGrenade(const ObjRectangle& rect, std::shared_ptr<Window> window,
                           std::shared_ptr<EventSystem> events, const int durationMs, const int lifeTimeMs,
                           const int color, const int id, const GameMode gameMode)
	: Bonus{rect, std::move(window), std::move(events), durationMs, lifeTimeMs, color, "BonusGrenade", id, gameMode}
{
	_events->EmitEvent<const std::string&, const FPoint, const BonusTypeId, const int>(
			"ServerSend_BonusSpawn", _name, FPoint{rect.x, rect.y}, Grenade, _id);
}

BonusGrenade::~BonusGrenade() = default;

void BonusGrenade::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>("BonusGrenade", author, fraction);
}

void BonusGrenade::PickUpBonus(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>("BonusGrenade", author, fraction);
}
