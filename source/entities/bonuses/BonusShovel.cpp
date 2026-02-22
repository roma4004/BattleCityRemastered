#include "entities/Bonuses/BonusShovel.h"
#include "components/EventSystem.h"
#include "enums/BonusType.h"

BonusShovel::BonusShovel(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events,
						 const milliseconds lifeTime, const unsigned int color, const buuid uuid,
						 const GameMode gameMode, const milliseconds duration)
	: Bonus{rect, events, lifeTime, color, "BonusShovel", uuid, gameMode, BonusType::Shovel}
	, _effectDuration{duration} {}

BonusShovel::~BonusShovel() = default;

void BonusShovel::PickUpBonus(const std::string& /*author*/, const std::string& fraction)
{
	_events->EmitEvent(_name, fraction, _effectDuration);
}
