#pragma once

#include "IFortress.h"
#include "Obstacle.h"
#include <memory>
#include <string>

enum class Faction : char8_t;
struct ObjRectangle;
struct DrawEvent;
struct PostDrawEvent;
struct BonusShovelStatusChangeEvent;
class GameConfig;
class EventSystem;

class EagleTile final : public Obstacle, public IFortress
{
	static constexpr int kHealth{100};

	void Subscribe() override;
	void OnDraw(const DrawEvent&) const;
	void OnPostDraw(const PostDrawEvent&) const;
	void OnBonusShovel(const BonusShovelStatusChangeEvent& event);

protected:
	void EmitDeathStatistics(Author author) override;
	void OnDespawned(const DespawnedEvent& event) override;

	static constexpr CollisionTags kCollision{tags::Impassable{}, tags::Destructible{}, tags::Impenetrable{},
											  tags::NoTerrain{}};

public:
	EagleTile(ObjRectangle rect, const std::shared_ptr<EventSystem>& events, Uuid uuid, const GameConfig& gameConfig);
};
