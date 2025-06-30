#include "entities/bonuses/BonusGrenade.h"
#include "enums/BonusType.h"

BonusGrenade::BonusGrenade(const ObjRectangle& rect, std::shared_ptr<EventSystem> events, const milliseconds duration,
                           const milliseconds lifeTime, const int color, const buuid uuid, const GameMode gameMode)
	: Bonus{rect,
	        std::move(events),
	        duration,
	        lifeTime,
	        color,
	        "BonusGrenade",
	        uuid,
	        gameMode,
	        BonusType::Grenade} {}

BonusGrenade::~BonusGrenade() = default;
