#pragma once

#include "Bonus.h"
#include <memory>

struct UPoint;
class BaseObj;
class EventSystem;

class BonusShovel final : public Bonus
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

	milliseconds _effectDuration{};

public:
	BonusShovel(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events, milliseconds lifeTime,
				buuid uuid, GameMode gameMode, milliseconds duration);

	~BonusShovel() override;

protected:
	void EmitPickupEvent(const std::string& author, const std::string& fraction) override;
};
