#include "entities/bonuses/BonusCaliber.h"
#include "components/EventSystem.h"
#include "components/events/BonusPickupEvents.h"
#include "enums/BonusType.h"

BonusCaliber::BonusCaliber(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events,
						   const milliseconds lifeTime, const Uuid uuid, const GameMode gameMode)
	: Bonus{rect,
			events,
			lifeTime,
			"BonusCaliber",
			uuid,
			gameMode,
			BonusType::Caliber} {}

BonusCaliber::~BonusCaliber() = default;

void BonusCaliber::EmitPickupEvent(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent(BonusCaliberPickupEvent{.author = author, .fraction = fraction});
}
