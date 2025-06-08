#pragma once

#include "Bonus.h"

#include <memory>

struct UPoint;
class BaseObj;
class EventSystem;

class BonusStar final : public Bonus
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

public:
	BonusStar(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
	          milliseconds duration, milliseconds lifeTime, int color, buuid uuid, GameMode gameMode);

	~BonusStar() override;
};
