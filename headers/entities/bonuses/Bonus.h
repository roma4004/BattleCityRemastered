#pragma once

#include "../BaseObj.h"
#include "../Tags.h"
#include "components/EventSystem.h"
#include "interfaces/IDrawable.h"
#include "interfaces/IPickupableBonus.h"
#include <memory>
#include <string>
#include <vector>

enum class Faction : char8_t;
enum class GameMode : char8_t;
enum class BonusType : char8_t;
enum class DespawnReason : char8_t;
struct BaseObjProperty;
class EventSystem;
struct DrawEvent;
struct DespawnedEvent;

class Bonus final : public BaseObj, public IDrawable, public IPickupableBonus
{
public:
	static constexpr CollisionTags kCollision{tags::Impassable{}, tags::Destructible{}, tags::Impenetrable{}};

private:
	GameMode _gameMode{};
	BonusType _bonusType{};
	bool _isSuper{};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	void Draw() const override;
	void OnDraw(const DrawEvent&) const;
	void OnDespawned(const DespawnedEvent& event);
	void Despawn(DespawnReason reason);

	void EmitDamageStatistics(Author author) override;

public:
	Bonus(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events, Uuid uuid, GameMode gameMode,
		  BonusType bonusType, bool isSuper);

	~Bonus() override;

	void Activate() override;
	void Deactivate() override;

	void Subscribe();
	void SubscribeAsClient();
	void Expire();

	//BaseObj overrides
	void TakeDamage(unsigned int damage, Author author) override;

	void PickUpBonus(Author author) override;

	[[nodiscard]] bool GetIsSuper() const;
};
