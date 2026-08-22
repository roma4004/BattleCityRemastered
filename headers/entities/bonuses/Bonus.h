#pragma once

#include "../BaseObj.h"
#include "../Tags.h"
#include "components/EventSystem.h"
#include "interfaces/IDrawable.h"
#include "interfaces/IPickupableBonus.h"
#include "interfaces/ITickUpdatable.h"
#include "utils/Timer.h"
#include <vector>

enum class GameMode : char8_t;
enum class BonusType : char8_t;
enum class DespawnReason : char8_t;
struct BaseObjProperty;
class EventSystem;
struct DrawEvent;
struct TickUpdateEvent;
struct DespawnedEvent;

class Bonus : public BaseObj, public IDrawable, public ITickUpdatable, public IPickupableBonus
{
	using milliseconds = std::chrono::milliseconds;
	Timer _lifeTimeTimer{};
	GameMode _gameMode{};
	BonusType _bonusType{};
	DespawnReason _despawnReason{};

protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	void TickUpdate(double deltaTime) override;
	void Draw() const override;
	void OnDraw(const DrawEvent&) const;
	void OnTickUpdate(const TickUpdateEvent& event);
	void OnDespawned(const DespawnedEvent& event);

	virtual void EmitPickupEvent(const std::string& author, const std::string& fraction) = 0;

public:
	static constexpr CollisionTags s_collision{tags::Impassable{}, tags::Destructible{}, tags::Impenetrable{}};

	Bonus(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events, milliseconds lifeTime,
		  std::string name, Uuid uuid, GameMode gameMode, BonusType bonusType);

	~Bonus() override;

	void Subscribe();
	void SubscribeAsAuthority();
	void SubscribeAsClient();

	//BaseObj overrides
	void TakeDamage(unsigned int damage, const std::string& author, const std::string& fraction) override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;
	void PickUpBonus(const std::string& author, const std::string& fraction) override;
};
