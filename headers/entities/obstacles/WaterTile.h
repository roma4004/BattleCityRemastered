#pragma once

#include "Obstacle.h"
#include <memory>

struct ObjRectangle;
class EventSystem;

class WaterTile final : public Obstacle
{
	using buuid = boost::uuids::uuid;

public:
	WaterTile(ObjRectangle rect, std::shared_ptr<EventSystem> events, buuid uuid, GameMode gameMode);

	~WaterTile() override;

	int animFrame{0};
	int animFrameCounter{0};
};
