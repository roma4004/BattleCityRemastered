#pragma once

#include "Obstacle.h"
#include <memory>
#include <string>

enum class Faction : char8_t;
struct DrawEvent;

class SteelWall : public Obstacle
{
	void Subscribe();
	void OnDraw(const DrawEvent&) const;

protected:
	void EmitDeathStatistics(const std::string& author, Faction faction) override;

	static constexpr CollisionTags kCollision{tags::Impassable{}, tags::Indestructible{}, tags::Impenetrable{}};

public:
	SteelWall(ObjRectangle rect, const std::shared_ptr<EventSystem>& events, Uuid uuid, GameMode gameMode);

	~SteelWall() override = default;
};
