#pragma once

#include "../BaseObj.h"
#include "../Tags.h"
#include "components/EventSystem.h"
#include "interfaces/IDrawable.h"
#include "interfaces/IPickupableBonus.h"
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
	DespawnReason _despawnReason{};
	bool _isSuper{};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	void Draw() const override;
	void OnDraw(const DrawEvent&) const;
	void OnDespawned(const DespawnedEvent& event);

	void EmitDamageStatistics(const std::string& author, Faction faction) override;

public:
	Bonus(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events, Uuid uuid, GameMode gameMode,
		  BonusType bonusType, bool isSuper);

	~Bonus() override;

	void Subscribe();
	void SubscribeAsClient();
	void Expire();

	//BaseObj overrides
	void TakeDamage(unsigned int damage, const std::string& author, Faction faction) override;

	void PickUpBonus(const std::string& author, Faction faction) override;

	[[nodiscard]] bool GetIsSuper() const;
};
