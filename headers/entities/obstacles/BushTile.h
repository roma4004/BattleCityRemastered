#pragma once

#include "Obstacle.h"
#include <memory>

struct ObjRectangle;
class EventSystem;
struct PostDrawEvent;

class BushTile final : public Obstacle
{
	using buuid = boost::uuids::uuid;

	void Subscribe() override;
	void OnPostDraw(const PostDrawEvent&);

protected:
	//NOTE: Bush is indestructible - never reaches SendDamageStatistics's health<1 branch, but the
	//base hook is pure virtual so every leaf must still supply a (no-op) body.
	void EmitDeathStatistics(const std::string& author, const std::string& fraction) override;

public:
	static constexpr CollisionTags kCollision{tags::Passable{}, tags::Indestructible{}, tags::Penetrable{}};

	BushTile(ObjRectangle rect, const std::shared_ptr<EventSystem>& events, buuid uuid, GameMode gameMode);

	~BushTile() override = default;
};
