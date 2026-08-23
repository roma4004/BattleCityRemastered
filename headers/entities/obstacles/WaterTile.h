#pragma once

#include "Obstacle.h"
#include <memory>

struct ObjRectangle;
class EventSystem;

class WaterTile final : public Obstacle
{
protected:
	void EmitDeathStatistics(const std::string& author, const std::string& fraction) override;

	static constexpr CollisionTags s_collision{tags::Impassable{}, tags::Indestructible{}, tags::Penetrable{}};

public:
	WaterTile(ObjRectangle rect, const std::shared_ptr<EventSystem>& events, Uuid uuid, GameMode gameMode);

	~WaterTile() override;
};
