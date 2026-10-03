#pragma once

#include "Obstacle.h"
#include <memory>
#include <string>

enum class Faction : char8_t;
struct ObjRectangle;
class GameConfig;
class EventSystem;

class IceTile final : public Obstacle
{
protected:
	void EmitDeathStatistics(Author author) override;

	static constexpr CollisionTags kCollision{tags::Passable{}, tags::Indestructible{}, tags::Penetrable{},
											  tags::Ice{}};
	static constexpr DrawLayer kLayer{DrawLayer::Ground};

public:
	IceTile(ObjRectangle rect, const std::shared_ptr<EventSystem>& events, Uuid uuid, const GameConfig& gameConfig);

	~IceTile() override = default;
};
