#pragma once

#include "Bonus.h"
#include <memory>

struct UPoint;
class BaseObj;
class EventSystem;

class BonusCaliber final : public Bonus
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

public:
	BonusCaliber(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events, milliseconds lifeTime,
				 buuid uuid, GameMode gameMode);

	~BonusCaliber() override;

protected:
	void EmitPickupEvent(const std::string& author, const std::string& fraction) override;
};
