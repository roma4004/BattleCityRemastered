#pragma once

#include "Obstacle.h"
#include <memory>

struct ObjRectangle;
class EventSystem;

class EagleTile final : public Obstacle
{
	using buuid = boost::uuids::uuid;

	void Subscribe() override;

protected:
	//NOTE: Eagle's death is handled separately via PlayersBaseFinishedEvent (see the destructor),
	//not the BrickWall/SteelWall-style death-statistics struct - no-op body to satisfy the base's
	//pure virtual hook.
	void EmitDeathStatistics(const std::string& author, const std::string& fraction) override;

public:
	static constexpr CollisionTags kCollision{tags::Impassable{}, tags::Destructible{}, tags::Impenetrable{}};

	EagleTile(ObjRectangle rect, const std::shared_ptr<EventSystem>& events, buuid uuid, GameMode gameMode);

	~EagleTile() override;
};
