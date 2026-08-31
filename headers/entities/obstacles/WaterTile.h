#pragma once

#include "Obstacle.h"
#include <memory>
#include <string>

enum class Faction : char8_t;
struct ObjRectangle;
class EventSystem;

class WaterTile final : public Obstacle
{
protected:
	void EmitDeathStatistics(const std::string& author, Faction faction) override;

	static constexpr CollisionTags kCollision{tags::Impassable{}, tags::Indestructible{}, tags::Penetrable{}};

public:
	WaterTile(ObjRectangle rect, const std::shared_ptr<EventSystem>& events, Uuid uuid, GameMode gameMode);

	~WaterTile() override;
};
