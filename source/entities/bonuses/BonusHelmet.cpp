#include "entities/bonuses/BonusHelmet.h"
#include "components/EventSystem.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/ObstacleAndBonusEvents.h"
#include "enums/BonusType.h"
#include "enums/GameMode.h"

BonusHelmet::BonusHelmet(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events,
						 const milliseconds lifeTime, const Uuid uuid, const GameMode gameMode,
						 const milliseconds duration)
	: Bonus{rect, events, lifeTime, "BonusHelmet", uuid, gameMode, BonusType::Helmet}
	, _effectDuration{duration} {}

BonusHelmet::~BonusHelmet() = default;

void BonusHelmet::EmitPickupEvent(const std::string& author, const std::string&)
{
	_events->EmitEvent(BonusHelmetPickupEvent{.author = author, .effectDuration = _effectDuration});
}
