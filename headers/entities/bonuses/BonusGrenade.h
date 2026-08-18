#pragma once

#include "Bonus.h"
#include <memory>

struct UPoint;
class BaseObj;
class EventSystem;

class BonusGrenade final : public Bonus
{
	using milliseconds = std::chrono::milliseconds;
public:
	BonusGrenade(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events, milliseconds lifeTime,
				 Uuid uuid, GameMode gameMode);

	~BonusGrenade() override;

protected:
	void EmitPickupEvent(const std::string& author, const std::string& fraction) override;
};
