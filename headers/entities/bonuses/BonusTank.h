#pragma once

#include "Bonus.h"
#include <memory>

struct UPoint;
struct ObjRectangle;
class BaseObj;
class EventSystem;

class BonusTank final : public Bonus
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

public:
	BonusTank(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events, milliseconds lifeTime,
			  buuid uuid, GameMode gameMode);

	~BonusTank() override;
};
