#pragma once

#include "Obstacle.h"
#include <memory>

class SteelWall final : public Obstacle
{
	using buuid = boost::uuids::uuid;

	void Subscribe() override;
	void Unsubscribe() const override;

public:
	SteelWall(ObjRectangle rect, std::shared_ptr<EventSystem> events, buuid uuid, GameMode gameMode);
	SteelWall(ObjRectangle rect, std::shared_ptr<EventSystem> events, buuid uuid, GameMode gameMode,
	          bool isReplicationOn);

	~SteelWall() override;
};
