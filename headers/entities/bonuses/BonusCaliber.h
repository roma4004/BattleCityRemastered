#pragma once

#include "Bonus.h"
#include <memory>

struct UPoint;
class BaseObj;
class EventSystem;

class BonusCaliber final : public Bonus
{
	using milliseconds = std::chrono::milliseconds;
public:
	BonusCaliber(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events, milliseconds lifeTime,
				 Uuid uuid, GameMode gameMode);

	~BonusCaliber() override;

protected:
	void EmitPickupEvent(const std::string& author, const std::string& fraction) override;
};
