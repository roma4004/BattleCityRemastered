#pragma once

#include "Bonus.h"
#include <memory>

struct UPoint;
class BaseObj;
class EventSystem;

class BonusHelmet final : public Bonus
{
	using milliseconds = std::chrono::milliseconds;
	milliseconds _effectDuration{};

public:
	BonusHelmet(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events, milliseconds lifeTime,
				Uuid uuid, GameMode gameMode, milliseconds duration);

	~BonusHelmet() override;

protected:
	void EmitPickupEvent(const std::string& author, const std::string& fraction) override;
};
