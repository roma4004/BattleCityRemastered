#pragma once

#include "Obstacle.h"
#include <memory>

struct ObjRectangle;
class EventSystem;
struct PreDrawEvent;

class IceTile final : public Obstacle
{
	void Subscribe();
	void OnPreDraw(const PreDrawEvent&) const;

protected:
	void EmitDeathStatistics(const std::string& author, const std::string& fraction) override;

	static constexpr CollisionTags s_collision{tags::Passable{}, tags::Indestructible{}, tags::Penetrable{}};

public:
	IceTile(ObjRectangle rect, const std::shared_ptr<EventSystem>& events, Uuid uuid, GameMode gameMode);

	~IceTile() override = default;
};
