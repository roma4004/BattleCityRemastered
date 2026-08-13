#pragma once

#include "Obstacle.h"
#include <memory>

struct DrawEvent;
class FortressWall;

class SteelWall final : public Obstacle
{
	using buuid = boost::uuids::uuid;
	friend FortressWall;

	void Subscribe() override;
	void OnDraw(const DrawEvent&) const;

protected:
	void EmitDeathStatistics(const std::string& author, const std::string& fraction) override;

	static constexpr CollisionTags s_collision{tags::Impassable{}, tags::Indestructible{}, tags::Impenetrable{}};

public:
	SteelWall(ObjRectangle rect, const std::shared_ptr<EventSystem>& events, buuid uuid, GameMode gameMode);

	~SteelWall() override = default;
};
