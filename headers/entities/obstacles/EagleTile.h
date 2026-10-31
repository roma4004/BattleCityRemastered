#pragma once

#include "IFortress.h"
#include "Obstacle.h"
#include <memory>
#include <optional>
#include <string>

enum class Faction : char8_t;
struct ObjRectangle;
struct BonusShovelPickupEvent;
struct BonusShovelStatusChangeEvent;
class GameConfig;
class EventSystem;

class EagleTile final : public Obstacle, public IFortress
{
	static constexpr unsigned int kHealth{100};

	void Subscribe() override;
	void OnBonusShovel(const BonusShovelStatusChangeEvent& event);
	void OnBonusShovelPickup(const BonusShovelPickupEvent& event);

protected:
	void EmitDeathStatistics(Author author) override;
	void OnDespawned(const DespawnedEvent& event) override;

	static constexpr CollisionTags kCollision{tags::Impassable{}, tags::Destructible{}, tags::Impenetrable{},
											  tags::NoTerrain{}};
	static constexpr DrawLayer kLayer{DrawLayer::World};

public:
	EagleTile(ObjRectangle rect, const std::shared_ptr<EventSystem>& events, Uuid uuid, const GameConfig& gameConfig);

	[[nodiscard]] std::optional<unsigned int> ShownHealth() const override;
};
