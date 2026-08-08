#pragma once

#include "Bonus.h"
#include <memory>

struct UPoint;
class BaseObj;
class EventSystem;

class BonusGrenade final : public Bonus
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

public:
	BonusGrenade(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events, milliseconds lifeTime,
				 buuid uuid, GameMode gameMode);

	~BonusGrenade() override;

protected:
	void EmitPickupEvent(const std::string& author, const std::string& fraction) override;
};
