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
	void OnPostDraw(const PostDrawEvent&) const;

protected:
	//NOTE: Bush is indestructible - never reaches SendDamageStatistics's health<1 branch, but the
	//base hook is pure virtual so every leaf must still supply a (no-op) body.
	void EmitDeathStatistics(const std::string& author, const std::string& fraction) override;

	static constexpr CollisionTags s_collision{tags::Passable{}, tags::Indestructible{}, tags::Penetrable{}};

public:
	BushTile(ObjRectangle rect, const std::shared_ptr<EventSystem>& events, buuid uuid, GameMode gameMode);

	~BushTile() override = default;
};
