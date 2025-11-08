#pragma once

#include "Obstacle.h"
#include <memory>

struct ObjRectangle;
class EventSystem;

class BrickWall final : public Obstacle
{
	using buuid = boost::uuids::uuid;

	void Subscribe() override;
	void Unsubscribe() const override;

public:
	BrickWall(ObjRectangle rect, const std::shared_ptr<EventSystem>& events, buuid uuid, GameMode gameMode);
	BrickWall(ObjRectangle rect, const std::shared_ptr<EventSystem>& events, buuid uuid, GameMode gameMode,
	          bool isReplicationOn);

	~BrickWall() override;
};
