#include "entities/Bonuses/BonusTimer.h"
#include "components/EventSystem.h"
#include "enums/BonusType.h"

//TODO: replicate timer bonus effect end
//TODO: on timer bonus effect enemy team should not shoot

BonusTimer::BonusTimer(const ObjRectangle& rect, std::shared_ptr<EventSystem> events, const milliseconds duration,
                       const milliseconds lifeTime, const int color, const buuid uuid, const GameMode gameMode)
	: Bonus{rect,
	        std::move(events),
	        duration,
	        lifeTime,
	        color,
	        "BonusTimer",
	        uuid,
	        gameMode,
	        BonusType::Timer} {}

BonusTimer::~BonusTimer() = default;

void BonusTimer::PickUpBonus(const std::string& /*author*/, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const milliseconds>("TimerActive", fraction, _effectDuration);
}
