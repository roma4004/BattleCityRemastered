#include "../../headers/bonuses/BonusCaliber.h"
#include "../../headers/enums/BonusType.h"

BonusCaliber::BonusCaliber(const ObjRectangle& rect, std::shared_ptr<EventSystem> events, const milliseconds duration,
                           const milliseconds lifeTime, const int color, const buuid uuid,
                           const GameMode gameMode)
	: Bonus{rect,
	        std::move(events),
	        duration,
	        lifeTime,
	        color,
	        "BonusCaliber",
	        uuid,
	        gameMode,
	        Caliber} {}

BonusCaliber::~BonusCaliber() = default;
