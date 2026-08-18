#include "entities/bonuses/BonusTank.h"
#include "components/EventSystem.h"
#include "components/events/BonusPickupEvents.h"
#include "enums/BonusType.h"

BonusTank::BonusTank(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events, const milliseconds lifeTime,
					 const Uuid uuid, const GameMode gameMode)
	: Bonus{rect, events, lifeTime, "BonusTank", uuid, gameMode, BonusType::Tank} {}

BonusTank::~BonusTank() = default;

void BonusTank::EmitPickupEvent(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent(BonusTankPickupEvent{.author = author, .fraction = fraction});
}
