#include "entities/Bonuses/BonusTimer.h"
#include "components/EventSystem.h"
#include "components/events/ObstacleAndBonusEvents.h"
#include "enums/BonusType.h"
#include "enums/GameMode.h"

BonusTimer::BonusTimer(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events,
					   const milliseconds lifeTime, const buuid uuid, const GameMode gameMode,
					   const milliseconds duration)
	: Bonus{rect, events, lifeTime, "BonusTimer", uuid, gameMode, BonusType::Timer}
	, _effectDuration{duration} {}

BonusTimer::~BonusTimer() = default;

void BonusTimer::PickUpBonus(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent("Statistics_BonusPickup", StatisticsAttributionEvent{author, fraction});
	_events->EmitEvent(_name + "_Pickup", fraction, _effectDuration);
	TakeDamage(GetHealth(), _name, _fraction);
}
