#pragma once

#include "Obstacle.h"
#include <memory>

struct ObjRectangle;
class EventSystem;

class GrassTile final : public Obstacle
{
	using buuid = boost::uuids::uuid;

	void Subscribe() override;
	void Unsubscribe() const override;

public:
	GrassTile(ObjRectangle rect, std::shared_ptr<EventSystem> events, buuid uuid, GameMode gameMode);

	~GrassTile() override;
};
