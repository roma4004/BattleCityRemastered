#include "entities/bonuses/BonusGrenade.h"
#include "enums/BonusType.h"

BonusGrenade::BonusGrenade(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events,
                           const milliseconds lifeTime, const unsigned int color, const buuid uuid,
                           const GameMode gameMode)
	: Bonus{rect,
	        events,
	        lifeTime,
	        color,
	        "BonusGrenade",
	        uuid,
	        gameMode,
	        BonusType::Grenade} {}

BonusGrenade::~BonusGrenade() = default;
