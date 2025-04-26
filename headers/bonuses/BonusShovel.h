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
	            std::chrono::milliseconds duration, std::chrono::milliseconds lifeTime, int color, int id,
	            GameMode gameMode);

	~BonusShovel() override;

	void PickUpBonus(const std::string& author, const std::string& fraction) override;
};
