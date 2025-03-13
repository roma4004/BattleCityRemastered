#pragma once

#include "Bonus.h"

#include <memory>

struct UPoint;
class BaseObj;
class EventSystem;

class BonusGrenade final : public Bonus
{
public:
	BonusGrenade(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
	             std::chrono::milliseconds duration, std::chrono::milliseconds lifeTime, int color, int id,
	             GameMode gameMode);

	~BonusGrenade() override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

	void PickUpBonus(const std::string& author, const std::string& fraction) override;
};
