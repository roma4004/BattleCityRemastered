#include "../../headers/Bonuses/BonusShovel.h"
#include "../../headers/EventSystem.h"
#include "../../headers/GameMode.h"
#include "../../headers/Point.h"
#include "../../headers/bonuses/BonusType.h"

BonusShovel::BonusShovel(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                         const std::chrono::milliseconds duration, const std::chrono::milliseconds lifeTime,
                         const int color, const int id, const GameMode gameMode)
	: Bonus{rect, std::move(window), std::move(events), duration, lifeTime, color, "BonusShovel", id, gameMode}
{
	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string&, const FPoint, const BonusType, const int>(
				"ServerSend_BonusSpawn", _name, FPoint{rect.x, rect.y}, Shovel, _id);
	}
}

BonusShovel::~BonusShovel() = default;

void BonusShovel::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>("BonusShovel", author, fraction);
}

void BonusShovel::PickUpBonus(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&, const std::chrono::milliseconds>(
			"BonusShovel", author, fraction, _duration);
}
