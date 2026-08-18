#pragma once

#include "Obstacle.h"
#include <memory>

struct ObjRectangle;
struct DrawEvent;
class EventSystem;
class FortressWall;

class BrickWall final : public Obstacle
{
	friend FortressWall;

	void Subscribe() override;
	void OnDraw(const DrawEvent&) const;

protected:
	void EmitDeathStatistics(const std::string& author, const std::string& fraction) override;

	static constexpr CollisionTags s_collision{tags::Impassable{}, tags::Destructible{}, tags::Impenetrable{}};

public:
	BrickWall(ObjRectangle rect, const std::shared_ptr<EventSystem>& events, Uuid uuid, GameMode gameMode);

	~BrickWall() override = default;
};
