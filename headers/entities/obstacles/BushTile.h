#pragma once

#include "Obstacle.h"
#include <memory>
#include <string>

enum class Faction : char8_t;
struct ObjRectangle;
struct PostDrawEvent;
class GameConfig;
class EventSystem;

class BushTile final : public Obstacle
{
	void Subscribe() override;
	void OnPostDraw(const PostDrawEvent&) const;

protected:
	void EmitDeathStatistics(Author author) override;

	static constexpr CollisionTags kCollision{tags::Passable{}, tags::Indestructible{}, tags::Penetrable{}};

public:
	BushTile(ObjRectangle rect, const std::shared_ptr<EventSystem>& events, Uuid uuid, const GameConfig& gameConfig);

	~BushTile() override = default;
};
