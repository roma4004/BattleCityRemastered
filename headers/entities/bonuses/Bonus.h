#pragma once

#include "../BaseObj.h"
#include "../Tags.h"
#include "components/EventSystem.h"
#include "components/Sprite.h"
#include "enums/DrawLayer.h"
#include "interfaces/IPickupableBonus.h"
#include <memory>
#include <optional>
#include <vector>

enum class Faction : char8_t;
enum class BonusType : char8_t;
enum class DespawnReason : char8_t;
struct BaseObjProperty;
struct DespawnedEvent;
struct WorldSnapshotRequestedEvent;
class EventSystem;
class GameConfig;

class Bonus final : public BaseObj, public IPickupableBonus
{
public:
	static constexpr CollisionTags kCollision{tags::Impassable{}, tags::Destructible{}, tags::Impenetrable{},
											  tags::NoTerrain{}};
	static constexpr DrawLayer kLayer{DrawLayer::World};

private:
	const GameConfig& _gameConfig;
	BonusType _bonusType{};
	bool _isSuper{};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	void OnDespawned(const DespawnedEvent& event);
	void OnWorldSnapshotRequested(const WorldSnapshotRequestedEvent& event) const;
	void Subscribe();
	void SubscribeAsClient();
	void Despawn(DespawnReason reason);

	void EmitDamageStatistics(Author author) override;

public:
	Bonus(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events, Uuid uuid,
		  const GameConfig& gameConfig, BonusType bonusType, bool isSuper);

	void Activate() override;
	void Deactivate() override;

	void Expire();

	void TakeDamage(unsigned int damage, Author author) override;

	void PickUpBonus(Author author) override;

	[[nodiscard]] bool GetIsSuper() const noexcept;

	[[nodiscard]] std::optional<Sprite> Look() const override;
};
