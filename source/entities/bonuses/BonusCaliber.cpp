#include "entities/bonuses/BonusCaliber.h"
#include "enums/BonusType.h"

BonusCaliber::BonusCaliber(const ObjRectangle& rect, std::shared_ptr<EventSystem> events, const milliseconds lifeTime,
                           const int color, const buuid uuid, const GameMode gameMode)
	: Bonus{rect,
	        std::move(events),
	        lifeTime,
	        color,
	        "BonusCaliber",
	        uuid,
	        gameMode,
	        BonusType::Caliber} {}

BonusCaliber::~BonusCaliber() = default;
