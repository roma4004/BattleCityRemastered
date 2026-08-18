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
public:
	BonusTank(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events, milliseconds lifeTime,
			  Uuid uuid, GameMode gameMode);

	~BonusTank() override;

protected:
	void EmitPickupEvent(const std::string& author, const std::string& fraction) override;
};
