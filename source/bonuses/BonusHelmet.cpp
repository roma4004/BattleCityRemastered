#include "../../headers/bonuses/BonusHelmet.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/BonusType.h"

BonusHelmet::BonusHelmet(const ObjRectangle& rect, std::shared_ptr<EventSystem> events, const milliseconds duration,
                         const milliseconds lifeTime, const int color, const buuid uuid, const GameMode gameMode)
	: Bonus{rect,
	        std::move(events),
	        duration,
	        lifeTime,
	        color,
	        "BonusHelmet",
	        uuid,
	        gameMode,
	        Helmet} {}

BonusHelmet::~BonusHelmet() = default;

void BonusHelmet::PickUpBonus(const std::string& author, const std::string& /*fraction*/)
{
	_events->EmitEvent<const std::string&, const milliseconds>("HelmetActive", author, _effectDuration);
}
