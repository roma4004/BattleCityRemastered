#pragma once

#include "Obstacle.h"
#include <memory>

enum class Faction : char8_t;
struct ObjRectangle;
class EventSystem;
struct PostDrawEvent;

class BushTile final : public Obstacle
{
	void Subscribe();
	void OnPostDraw(const PostDrawEvent&) const;

protected:
	void EmitDeathStatistics(const std::string& author, Faction faction) override;

	static constexpr CollisionTags kCollision{tags::Passable{}, tags::Indestructible{}, tags::Penetrable{}};

public:
	BushTile(ObjRectangle rect, const std::shared_ptr<EventSystem>& events, Uuid uuid, GameMode gameMode);

	~BushTile() override = default;
};
