#include "entities/Bonuses/BonusStar.h"
#include "components/EventSystem.h"
#include "components/events/BonusPickupEvents.h"
#include "enums/BonusType.h"

BonusStar::BonusStar(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events, const milliseconds lifeTime,
					 const buuid uuid, const GameMode gameMode)
	: Bonus{rect, events, lifeTime, "BonusStar", uuid, gameMode, BonusType::Star} {}

BonusStar::~BonusStar() = default;

void BonusStar::EmitPickupEvent(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent(BonusStarPickupEvent{.author = author, .fraction = fraction});
}
