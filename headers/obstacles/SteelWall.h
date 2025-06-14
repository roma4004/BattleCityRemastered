#pragma once

#include "Obstacle.h"

struct ObjRectangle;
class EventSystem;

class SteelWall final : public Obstacle
{
	using buuid = boost::uuids::uuid;

public:
	SteelWall(ObjRectangle rect, std::shared_ptr<EventSystem> events, buuid uuid, GameMode gameMode);

	~SteelWall() override;
};
