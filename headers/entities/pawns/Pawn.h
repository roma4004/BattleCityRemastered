#pragma once

#include "../BaseObj.h"
#include "../Tags.h"
#include "components/EventSystem.h"
#include "interfaces/ITickUpdatable.h"
#include <vector>

enum class Direction : char8_t;
enum class GameMode : char8_t;
struct PawnProperty;
struct ObjRectangle;
class IMoveBeh;
class EventSystem;
class GameConfig;
struct ClientInHealthEvent;
struct TickUpdateEvent;

class Pawn : public BaseObj, public ITickUpdatable
{
public:
	Pawn(PawnProperty pawnProperty, GameConfig& gameConfig, CollisionTags collision);

	~Pawn() override;

	//BaseObj overrides
	void TakeDamage(unsigned int damage, const std::string& damageAuthor, const std::string& damageFraction) override;

	[[nodiscard]] Direction GetDirection() const;
	void SetDirection(Direction dir);

	[[nodiscard]] float GetSpeed() const;
	void SetSpeed(float speed);

protected:
	float _speed{};
	std::string _uuidStr{};
	unsigned short _tier{1u};
	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<IMoveBeh> _moveBeh{nullptr};
	Direction _dir{};
	GameMode _gameMode{};
	GameConfig& _gameConfig;

	// _subs: the Subscribe()/Unsubscribe() toggle group (SubscribeAsAuthority/SubscribeAsClient plus
	// whatever derived classes' own Subscribe() overrides push in) - shared with derived classes
	// since Tank/Bullet add their own listeners into this same inherited vector rather than
	// keeping a separate one, so one Unsubscribe() clears everything for the whole hierarchy.
	// _tickUpdateSub: toggled independently by SubscribeTickUpdate()/UnsubscribeTickUpdate() (e.g.
	// to pause ticking during a bonus-timer effect) without disturbing the rest of _subs.
	// Both mutable: Unsubscribe()/UnsubscribeTickUpdate() are const but must be able to clear them.
	mutable std::vector<EventSubscription> _subs{};
	mutable EventSubscription _tickUpdateSub{};

	virtual void Subscribe();
	void Unsubscribe() const;

	void SubscribeTickUpdate();
	void UnsubscribeTickUpdate() const;
	void OnTickUpdate(const TickUpdateEvent& event);

	//TODO: implement collision detection through quadtree
	void TickUpdate(double deltaTime) override = 0;

private:
	virtual void SubscribeAsAuthority();
	virtual void SubscribeAsClient();
	void OnClientInHealth(const ClientInHealthEvent& event);
};
