#pragma once

#include "Obstacle.h"
#include "components/Sprite.h"
#include <memory>
#include <optional>
#include <string>

enum class Faction : char8_t;
struct ObjRectangle;
class GameConfig;
class EventSystem;

class WaterTile final : public Obstacle
{
protected:
	void EmitDeathStatistics(Author author) override;

	static constexpr CollisionTags kCollision{tags::Impassable{}, tags::Indestructible{}, tags::Penetrable{},
											  tags::Water{}};
	static constexpr DrawLayer kLayer{DrawLayer::Ground};

public:
	WaterTile(ObjRectangle rect, const std::shared_ptr<EventSystem>& events, Uuid uuid, const GameConfig& gameConfig);

	[[nodiscard]] std::optional<Sprite> Look() const override { return std::nullopt; }
};
