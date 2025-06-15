#pragma once

#include "Obstacle.h"
#include <memory>

struct ObjRectangle;
class EventSystem;

class IceTile final : public Obstacle
{
	using buuid = boost::uuids::uuid;

public:
	IceTile(ObjRectangle rect, std::shared_ptr<EventSystem> events, buuid uuid, GameMode gameMode);

	~IceTile() override;
};
