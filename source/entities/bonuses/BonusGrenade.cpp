#include "entities/bonuses/BonusGrenade.h"
#include "components/EventSystem.h"
#include "components/events/BonusPickupEvents.h"
#include "enums/BonusType.h"

BonusGrenade::BonusGrenade(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events,
						   const milliseconds lifeTime, const Uuid uuid, const GameMode gameMode)
	: Bonus{rect, events, lifeTime, "BonusGrenade", uuid, gameMode, BonusType::Grenade} {}

BonusGrenade::~BonusGrenade() = default;

void BonusGrenade::EmitPickupEvent(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent(BonusGrenadePickupEvent{.author = author, .fraction = fraction});
}
