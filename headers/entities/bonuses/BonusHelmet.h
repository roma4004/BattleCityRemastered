#pragma once

#include "Bonus.h"
#include <memory>

struct UPoint;
class BaseObj;
class EventSystem;

class BonusHelmet final : public Bonus
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

public:
	BonusHelmet(const ObjRectangle& rect, std::shared_ptr<EventSystem> events, milliseconds duration,
	            milliseconds lifeTime, int color, buuid uuid, GameMode gameMode);

	~BonusHelmet() override;

	void PickUpBonus(const std::string& author, const std::string& fraction) override;
};
