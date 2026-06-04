#pragma once

#include "Bonus.h"
#include <memory>

struct UPoint;
class BaseObj;
class EventSystem;

class BonusStar final : public Bonus
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

public:
	BonusStar(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events, milliseconds lifeTime,
			  buuid uuid, GameMode gameMode);

	~BonusStar() override;
};
