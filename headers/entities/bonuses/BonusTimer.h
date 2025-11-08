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

	milliseconds _effectDuration{};

public:
	BonusTimer(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events, milliseconds lifeTime, int color,
	           buuid uuid, GameMode gameMode, milliseconds duration);

	~BonusTimer() override;

	void PickUpBonus(const std::string& author, const std::string& fraction) override;
};
