#include "../../headers/bonuses/BonusHelmet.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/BonusType.h"

BonusHelmet::BonusHelmet(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                         const std::chrono::milliseconds duration, const std::chrono::milliseconds lifeTime,
                         const int color, const int id, const GameMode gameMode)
	: Bonus{rect,
	        std::move(window),
	        std::move(events),
	        duration,
	        lifeTime,
	        color,
	        "BonusHelmet",
	        id,
	        gameMode,
	        Helmet} {}

BonusHelmet::~BonusHelmet() = default;

void BonusHelmet::PickUpBonus(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&, const std::chrono::milliseconds>(
			_name, author, fraction, _duration);
}
