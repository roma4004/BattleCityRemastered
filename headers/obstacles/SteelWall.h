#pragma once

#include "Obstacle.h"
#include "../BaseObj.h"
#include "../application/Window.h"

struct ObjRectangle;
class EventSystem;

//TODO: implement that steel wall can be destroyer when player have 3 bonus stars
class SteelWall final : public Obstacle
{
public:
	SteelWall(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events, int id);

	~SteelWall() override = default;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;
};
