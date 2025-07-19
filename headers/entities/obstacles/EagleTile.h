#pragma once

#include "Obstacle.h"
#include <memory>

struct ObjRectangle;
class EventSystem;

class EagleTile final : public Obstacle
{
	using buuid = boost::uuids::uuid;

	void Subscribe() override;
	void Unsubscribe() const override;

public:
	EagleTile(ObjRectangle rect, std::shared_ptr<EventSystem> events, buuid uuid, GameMode gameMode);

	~EagleTile() override;
};
