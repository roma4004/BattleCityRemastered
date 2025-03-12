#pragma once

#include "Bonus.h"

#include <memory>

struct UPoint;
class BaseObj;
class EventSystem;

class BonusShovel final : public Bonus
{
public:
	BonusShovel(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
	            int durationMs, int lifeTimeMs, int color, int id, GameMode gameMode);

	~BonusShovel() override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

	void PickUpBonus(const std::string& author, const std::string& fraction) override;
};
