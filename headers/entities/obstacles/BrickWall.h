#pragma once

#include "Obstacle.h"
#include <memory>

enum class Faction : char8_t;
struct ObjRectangle;
struct DrawEvent;
class EventSystem;

class BrickWall : public Obstacle
{
	void Subscribe();
	void OnDraw(const DrawEvent&) const;

protected:
	void EmitDeathStatistics(const std::string& author, Faction faction) override;

	static constexpr CollisionTags kCollision{tags::Impassable{}, tags::Destructible{}, tags::Impenetrable{}};

public:
	BrickWall(ObjRectangle rect, const std::shared_ptr<EventSystem>& events, Uuid uuid, GameMode gameMode);

	~BrickWall() override = default;
};
