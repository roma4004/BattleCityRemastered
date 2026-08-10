#include "entities/bonuses/BonusHelmet.h"
#include "components/EventSystem.h"
#include "enums/BonusType.h"
#include "enums/GameMode.h"

BonusHelmet::BonusHelmet(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events,
						 const milliseconds lifeTime, const buuid uuid, const GameMode gameMode,
						 const milliseconds duration)
	: Bonus{rect, events, lifeTime, "BonusHelmet", uuid, gameMode, BonusType::Helmet}
	, _effectDuration{duration} {}

BonusHelmet::~BonusHelmet() = default;

void BonusHelmet::PickUpBonus(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent("Statistics_BonusPickup", author, fraction);
	_events->EmitEvent(_name + "_Pickup", author, _effectDuration);
	TakeDamage(GetHealth(), _name, _fraction);
}
