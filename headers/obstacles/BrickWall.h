#pragma once

#include "Obstacle.h"

#include <memory>

struct ObjRectangle;
struct Window;
class EventSystem;

class BrickWall final : public Obstacle
{
public:
	BrickWall(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events, int id,
	          GameMode gameMode);

	~BrickWall() override = default;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;
};
