#include "../../headers/Bonuses/BonusTank.h"
#include "../../headers/enums/BonusType.h"

BonusTank::BonusTank(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                     const milliseconds duration, const milliseconds lifeTime, const int color, const buuid uuid,
                     const GameMode gameMode)
	: Bonus{rect,
	        std::move(window),
	        std::move(events),
	        duration,
	        lifeTime,
	        color,
	        "BonusTank",
	        uuid,
	        gameMode,
	        Tank} {}

BonusTank::~BonusTank() = default;
