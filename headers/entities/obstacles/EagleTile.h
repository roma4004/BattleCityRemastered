#pragma once

#include "IFortress.h"
#include "Obstacle.h"
#include <memory>
#include <string>

enum class Faction : char8_t;
struct ObjRectangle;
struct DrawEvent;
class GameConfig;
class EventSystem;

class EagleTile final : public Obstacle, public IFortress
{
	void Subscribe() override;
	void OnDraw(const DrawEvent&) const;

protected:
	//NOTE: empty - the eagle announces its death as PlayersBaseFinishedEvent from the destructor
	void EmitDeathStatistics(Author author) override;
	void OnDespawned(const DespawnedEvent& event) override;

	static constexpr CollisionTags kCollision{tags::Impassable{}, tags::Destructible{}, tags::Impenetrable{}};

public:
	EagleTile(ObjRectangle rect, const std::shared_ptr<EventSystem>& events, Uuid uuid, const GameConfig& gameConfig);
};
