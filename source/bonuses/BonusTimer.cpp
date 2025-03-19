#include "../../headers/Bonuses/BonusTimer.h"
#include "../../headers/EventSystem.h"
#include "../../headers/GameMode.h"
#include "../../headers/Point.h"
#include "../../headers/bonuses/BonusType.h"

BonusTimer::BonusTimer(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                       const std::chrono::milliseconds duration, const std::chrono::milliseconds lifeTime,
                       const int color, const int id, const GameMode gameMode)
	: Bonus{rect, std::move(window), std::move(events), duration, lifeTime, color, "BonusTimer", id, gameMode}
{
	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string&, const FPoint, const BonusType, const int>(
				"ServerSend_BonusSpawn", _name, FPoint{rect.x, rect.y}, Timer, _id);
	}
}

BonusTimer::~BonusTimer() = default;

void BonusTimer::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>("BonusTimer", author, fraction);
}

void BonusTimer::PickUpBonus(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&, const std::chrono::milliseconds>(
			"BonusTimer", author, fraction, _duration);
}
