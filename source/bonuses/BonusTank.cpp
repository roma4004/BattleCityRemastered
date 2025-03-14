#include "../../headers/Bonuses/BonusTank.h"
#include "../../headers/bonuses/BonusTypeId.h"

BonusTank::BonusTank(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                     const std::chrono::milliseconds duration, const std::chrono::milliseconds lifeTime,
                     const int color, const int id, const GameMode gameMode)
	: Bonus{rect, std::move(window), std::move(events), duration, lifeTime, color, "BonusTank", id, gameMode}
{
	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string&, const FPoint, const BonusTypeId, const int>(
				"ServerSend_BonusSpawn", _name, FPoint{rect.x, rect.y}, Tank, _id);
	}
}

BonusTank::~BonusTank() = default;

void BonusTank::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>("BonusTank", author, fraction);
}

void BonusTank::PickUpBonus(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>("BonusTank", author, fraction);
}
