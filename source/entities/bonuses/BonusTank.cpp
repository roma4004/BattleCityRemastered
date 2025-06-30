#include "entities/Bonuses/BonusTank.h"
#include "enums/BonusType.h"

BonusTank::BonusTank(const ObjRectangle& rect, std::shared_ptr<EventSystem> events, const milliseconds duration,
                     const milliseconds lifeTime, const int color, const buuid uuid, const GameMode gameMode)
	: Bonus{rect,
	        std::move(events),
	        duration,
	        lifeTime,
	        color,
	        "BonusTank",
	        uuid,
	        gameMode,
	        BonusType::Tank} {}

BonusTank::~BonusTank() = default;
