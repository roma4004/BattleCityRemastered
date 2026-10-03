#pragma once

#include "Obstacle.h"
#include <memory>
#include <string>

enum class Faction : char8_t;
class GameConfig;

class SteelWall : public Obstacle
{
protected:
	void EmitDeathStatistics(Author author) override;

	static constexpr CollisionTags kCollision{tags::Impassable{}, tags::Indestructible{}, tags::Impenetrable{},
											  tags::NoTerrain{}};
	static constexpr DrawLayer kLayer{DrawLayer::World};

public:
	SteelWall(ObjRectangle rect, const std::shared_ptr<EventSystem>& events, Uuid uuid, const GameConfig& gameConfig);

	~SteelWall() override = default;
};
