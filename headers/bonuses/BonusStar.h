#pragma once

#include "Bonus.h"

#include <memory>

struct UPoint;
class BaseObj;
class EventSystem;

class BonusStar final : public Bonus
{
public:
	BonusStar(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
	          int durationMs, int lifeTimeMs, int color, int id, GameMode gameMode);

	~BonusStar() override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

	void PickUpBonus(const std::string& author, const std::string& fraction) override;
};
