#pragma once

#include "Bonus.h"

#include <memory>

struct UPoint;
struct ObjRectangle;
class BaseObj;
class EventSystem;

class BonusTimer final : public Bonus
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

public:
	BonusTimer(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
	           milliseconds duration, milliseconds lifeTime, int color, buuid uuid, GameMode gameMode);

	~BonusTimer() override;

	void PickUpBonus(const std::string& author, const std::string& fraction) override;
};
