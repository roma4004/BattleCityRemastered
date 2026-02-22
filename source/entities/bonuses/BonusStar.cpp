#include "entities/Bonuses/BonusStar.h"
#include "enums/BonusType.h"

BonusStar::BonusStar(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events, const milliseconds lifeTime,
					 const unsigned int color, const buuid uuid, const GameMode gameMode)
	: Bonus{rect, events, lifeTime, color, "BonusStar", uuid, gameMode, BonusType::Star} {}

BonusStar::~BonusStar() = default;
