#include "entities/bonuses/BonusShovel.h"
#include "components/EventSystem.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/ObstacleAndBonusEvents.h"
#include "enums/BonusType.h"
#include "enums/GameMode.h"

BonusShovel::BonusShovel(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events,
						 const milliseconds lifeTime, const Uuid uuid, const GameMode gameMode,
						 const milliseconds duration)
	: Bonus{rect, events, lifeTime, "BonusShovel", uuid, gameMode, BonusType::Shovel}
	, _effectDuration{duration} {}

BonusShovel::~BonusShovel() = default;

void BonusShovel::EmitPickupEvent(const std::string&, const std::string& fraction)
{
	_events->EmitEvent(BonusShovelPickupEvent{.fraction = fraction, .effectDuration = _effectDuration});
}
