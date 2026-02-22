#include "entities/Bonuses/BonusTank.h"
#include "enums/BonusType.h"

BonusTank::BonusTank(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events, const milliseconds lifeTime,
					 const unsigned int color, const buuid uuid, const GameMode gameMode)
	: Bonus{rect, events, lifeTime, color, "BonusTank", uuid, gameMode, BonusType::Tank} {}

BonusTank::~BonusTank() = default;
