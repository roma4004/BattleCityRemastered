#pragma once

#include "Obstacle.h"
#include <memory>
#include <string>

enum class Faction : char8_t;
struct ObjRectangle;
class GameConfig;
class EventSystem;

class BrickWall : public Obstacle
{
protected:
	void EmitDeathStatistics(Author author) override;

	static constexpr CollisionTags kCollision{tags::Impassable{}, tags::Destructible{}, tags::Impenetrable{},
											  tags::NoTerrain{}};
	static constexpr DrawLayer kLayer{DrawLayer::World};

public:
	BrickWall(ObjRectangle rect, const std::shared_ptr<EventSystem>& events, Uuid uuid, const GameConfig& gameConfig);

	~BrickWall() override = default;
};
