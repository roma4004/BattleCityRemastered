#pragma once

#include "Bonus.h"

#include <memory>

struct UPoint;
class BaseObj;
class EventSystem;

class BonusGrenade final : public Bonus
{
	using buuid = boost::uuids::uuid;

public:
	BonusGrenade(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
	             std::chrono::milliseconds duration, std::chrono::milliseconds lifeTime, int color, buuid uuid,
	             GameMode gameMode);

	~BonusGrenade() override;
};
