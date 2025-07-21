#include "entities/Bonuses/BonusShovel.h"
#include "components/EventSystem.h"
#include "enums/BonusType.h"

BonusShovel::BonusShovel(const ObjRectangle& rect, std::shared_ptr<EventSystem> events, const milliseconds duration,
                         const milliseconds lifeTime, const int color, const buuid uuid, const GameMode gameMode)
	: Bonus{rect,
	        std::move(events),
	        duration,
	        lifeTime,
	        color,
	        "BonusShovel",
	        uuid,
	        gameMode,
	        BonusType::Shovel} {}

BonusShovel::~BonusShovel() = default;

void BonusShovel::PickUpBonus(const std::string& /*author*/, const std::string& fraction)
{
	_events->EmitEvent(_name, fraction, _effectDuration);
}
