#include "../../headers/Bonuses/BonusStar.h"
#include "../../headers/enums/BonusType.h"

BonusStar::BonusStar(const ObjRectangle& rect, std::shared_ptr<EventSystem> events, const milliseconds duration,
                     const milliseconds lifeTime, const int color, const buuid uuid,
                     const GameMode gameMode)
	: Bonus{rect,
	        std::move(events),
	        duration,
	        lifeTime,
	        color,
	        "BonusStar",
	        uuid,
	        gameMode,
	        Star} {}

BonusStar::~BonusStar() = default;
