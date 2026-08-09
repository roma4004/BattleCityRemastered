#pragma once

#include "Obstacle.h"
#include <memory>

struct ObjRectangle;
class EventSystem;

class BrickWall final : public Obstacle
{
	using buuid = boost::uuids::uuid;

	void Subscribe() override;

protected:
	void EmitDeathStatistics(const std::string& author, const std::string& fraction) override;

public:
	static constexpr CollisionTags kCollision{tags::Impassable{}, tags::Destructible{}, tags::Impenetrable{}};

	BrickWall(ObjRectangle rect, const std::shared_ptr<EventSystem>& events, buuid uuid, GameMode gameMode);

	~BrickWall() override = default;
};
