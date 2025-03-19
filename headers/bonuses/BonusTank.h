#pragma once

#include "Bonus.h"

#include <memory>

struct UPoint;
struct ObjRectangle;
class BaseObj;
class EventSystem;

class BonusTank final : public Bonus
{
public:
	BonusTank(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
	          std::chrono::milliseconds duration, std::chrono::milliseconds lifeTime, int color, int id,
	          GameMode gameMode);

	~BonusTank() override;
};
