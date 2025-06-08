#pragma once

#include "Obstacle.h"
#include <memory>

struct ObjRectangle;
struct Window;
class EventSystem;

class WaterTile final : public Obstacle
{
	using buuid = boost::uuids::uuid;

public:
	WaterTile(ObjRectangle rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events, buuid uuid,
	          GameMode gameMode, std::shared_ptr<IDrawable> textureManager);

	~WaterTile() override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;
};
