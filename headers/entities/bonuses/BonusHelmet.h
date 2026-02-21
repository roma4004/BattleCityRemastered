#pragma once

#include "Bonus.h"
#include <memory>

struct UPoint;
class BaseObj;
class EventSystem;

class BonusHelmet final : public Bonus
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

	milliseconds _effectDuration{};

public:
	BonusHelmet(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events, milliseconds lifeTime,
	            unsigned int color, buuid uuid, GameMode gameMode, milliseconds duration);

	~BonusHelmet() override;

	void PickUpBonus(const std::string& author, const std::string& fraction) override;
};
