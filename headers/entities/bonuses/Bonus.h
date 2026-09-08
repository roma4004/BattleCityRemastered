#pragma once

#include "../BaseObj.h"
#include "../Tags.h"
#include "components/EventSystem.h"
#include "interfaces/IDrawable.h"
#include "interfaces/IPickupableBonus.h"
#include <memory>
#include <vector>

enum class Faction : char8_t;
enum class BonusType : char8_t;
enum class DespawnReason : char8_t;
struct BaseObjProperty;
struct DrawEvent;
struct DespawnedEvent;
class EventSystem;
class GameConfig;

class Bonus final : public BaseObj, public IDrawable, public IPickupableBonus
{
public:
	static constexpr CollisionTags kCollision{tags::Impassable{}, tags::Destructible{}, tags::Impenetrable{}};

private:
	const GameConfig& _gameConfig;
	BonusType _bonusType{};
	bool _isSuper{};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	void Draw() const override;
	void OnDraw(const DrawEvent&) const;
	void OnDespawned(const DespawnedEvent& event);
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

	[[nodiscard]] bool GetIsSuper() const;
};
