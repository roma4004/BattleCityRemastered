#include "entities/bonuses/BonusTimer.h"
#include "components/EventSystem.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/ObstacleAndBonusEvents.h"
#include "enums/BonusType.h"
#include "enums/GameMode.h"

BonusTimer::BonusTimer(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events,
					   const milliseconds lifeTime, const Uuid uuid, const GameMode gameMode,
					   const milliseconds duration)
	: Bonus{rect, events, lifeTime, "BonusTimer", uuid, gameMode, BonusType::Timer}
	, _effectDuration{duration} {}

BonusTimer::~BonusTimer() = default;

void BonusTimer::EmitPickupEvent(const std::string&, const std::string& fraction)
{
	_events->EmitEvent(BonusTimerPickupEvent{.fraction = fraction, .effectDuration = _effectDuration});
}
