#include "../../headers/Bonuses/BonusShovel.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/BonusType.h"

BonusShovel::BonusShovel(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                         const milliseconds duration, const milliseconds lifeTime, const int color, const buuid uuid,
                         const GameMode gameMode)
	: Bonus{rect,
	        std::move(window),
	        std::move(events),
	        duration,
	        lifeTime,
	        color,
	        "BonusShovel",
	        uuid,
	        gameMode,
	        Shovel} {}

BonusShovel::~BonusShovel() = default;

void BonusShovel::PickUpBonus(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&, const milliseconds>(
			_name, author, fraction, _effectDuration);
}
