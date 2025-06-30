#pragma once

#include "Obstacle.h"
#include <memory>

struct ObjRectangle;
class EventSystem;

class BrickWall final : public Obstacle
{
	using buuid = boost::uuids::uuid;

public:
	BrickWall(ObjRectangle rect, std::shared_ptr<EventSystem> events, buuid uuid, GameMode gameMode);
	BrickWall(ObjRectangle rect, std::shared_ptr<EventSystem> events, buuid uuid, GameMode gameMode,
	          bool isReplicationOn);

	~BrickWall() override;
};
