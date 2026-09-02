#pragma once

#include "Obstacle.h"
#include <memory>
#include <string>

enum class Faction : char8_t;
struct ObjRectangle;
class EventSystem;
struct PreDrawEvent;

class IceTile final : public Obstacle
{
	void Subscribe() override;
	void OnPreDraw(const PreDrawEvent&) const;

protected:
	void EmitDeathStatistics(Author author) override;

	static constexpr CollisionTags kCollision{tags::Passable{}, tags::Indestructible{}, tags::Penetrable{}};

public:
	IceTile(ObjRectangle rect, const std::shared_ptr<EventSystem>& events, Uuid uuid, GameMode gameMode);

	~IceTile() override = default;
};
