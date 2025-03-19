#include "../../headers/Bonuses/BonusTimer.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/BonusType.h"

BonusTimer::BonusTimer(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                       const std::chrono::milliseconds duration, const std::chrono::milliseconds lifeTime,
                       const int color, const int id, const GameMode gameMode)
	: Bonus{rect,
	        std::move(window),
	        std::move(events),
	        duration,
	        lifeTime,
	        color,
	        "BonusTimer",
	        id,
	        gameMode,
	        Timer} {}

BonusTimer::~BonusTimer() = default;

void BonusTimer::PickUpBonus(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&, const std::chrono::milliseconds>(
			_name, author, fraction, _duration);
}
