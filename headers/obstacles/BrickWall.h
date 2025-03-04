#pragma once

#include "Obstacle.h"
#include "../ObjRectangle.h"
#include "../application/Window.h"

#include <memory>

class EventSystem;

class BrickWall final : public Obstacle
{
public:
	BrickWall(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events, int id);

	~BrickWall() override = default;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;
};
