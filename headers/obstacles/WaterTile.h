#pragma once

#include "Obstacle.h"
#include "../BaseObj.h"
#include "../Point.h"
#include "../application/Window.h"

#include <memory>

struct ObjRectangle;
class EventSystem;

class WaterTile final : public Obstacle
{
public:
	WaterTile(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events, int id,
	          GameMode gameMode);

	~WaterTile() override = default;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;
};
