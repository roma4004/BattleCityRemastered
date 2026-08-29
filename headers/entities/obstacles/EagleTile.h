#pragma once

#include "IFortress.h"
#include "Obstacle.h"
#include <memory>

enum class Faction : char8_t;
struct ObjRectangle;
class EventSystem;
struct DrawEvent;

class EagleTile final : public Obstacle, public IFortress
{
	void Subscribe();
	void OnDraw(const DrawEvent&) const;

protected:
	//NOTE: Eagle's death is handled separately via PlayersBaseFinishedEvent (see the destructor),
	//not the BrickWall/SteelWall-style death-statistics struct - no-op body to satisfy the base's
	//pure virtual hook.
	void EmitDeathStatistics(const std::string& author, Faction faction) override;
	void OnDespawned(const DespawnedEvent& event) override;

	static constexpr CollisionTags kCollision{tags::Impassable{}, tags::Destructible{}, tags::Impenetrable{}};

public:
	EagleTile(ObjRectangle rect, const std::shared_ptr<EventSystem>& events, Uuid uuid, GameMode gameMode);

	~EagleTile() override;
};
