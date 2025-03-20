#include "../../headers/Bonuses/BonusStar.h"
#include "../../headers/enums/BonusType.h"

BonusStar::BonusStar(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                     const std::chrono::milliseconds duration, const std::chrono::milliseconds lifeTime,
                     const int color, const int id, const GameMode gameMode)
	: Bonus{rect,
	        std::move(window),
	        std::move(events),
	        duration,
	        lifeTime,
	        color,
	        "BonusStar",
	        id,
	        gameMode,
	        Star} {}

BonusStar::~BonusStar() = default;
