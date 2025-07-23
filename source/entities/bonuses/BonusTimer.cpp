#include "entities/Bonuses/BonusTimer.h"
#include "components/EventSystem.h"
#include "enums/BonusType.h"

//TODO: replicate timer bonus effect end
//TODO: on timer bonus effect enemy team should not shoot

BonusTimer::BonusTimer(const ObjRectangle& rect, std::shared_ptr<EventSystem> events, const milliseconds lifeTime,
                       const int color, const buuid uuid, const GameMode gameMode, const milliseconds duration)
	: Bonus{rect,
	        std::move(events),
	        lifeTime,
	        color,
	        "BonusTimer",
	        uuid,
	        gameMode,
	        BonusType::Timer},
	  _effectDuration{duration} {}

BonusTimer::~BonusTimer() = default;

void BonusTimer::PickUpBonus(const std::string& /*author*/, const std::string& fraction)
{
	_events->EmitEvent(_name, fraction, _effectDuration);
}
