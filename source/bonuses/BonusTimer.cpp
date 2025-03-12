#include "../../headers/Bonuses/BonusTimer.h"
#include "../../headers/bonuses/BonusTypeId.h"

BonusTimer::BonusTimer(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                       const int durationMs, const int lifeTimeMs, const int color, const int id,
                       const GameMode gameMode)
	: Bonus{rect, std::move(window), std::move(events), durationMs, lifeTimeMs, color, "BonusTimer", id, gameMode}
{
	_events->EmitEvent<const std::string&, const FPoint, const BonusTypeId, const int>(
			"ServerSend_BonusSpawn", _name, FPoint{rect.x, rect.y}, Timer, _id);
}

BonusTimer::~BonusTimer() = default;

void BonusTimer::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>("BonusTimer", author, fraction);
}

void BonusTimer::PickUpBonus(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&, int>("BonusTimer", author, fraction, _bonusDurationMs);
}
