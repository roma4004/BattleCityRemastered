#pragma once

#include "../BaseObj.h"
#include "../Tags.h"
#include "components/EventSystem.h"
#include "interfaces/ITickUpdatable.h"
#include <memory>
#include <vector>

enum class Faction : char8_t;
enum class Direction : char8_t;
struct PawnProperty;
struct ObjRectangle;
struct HealthChangedEvent;
struct DespawnedEvent;
struct PosChangedEvent;
struct TickUpdateEvent;
class IMoveBeh;
class EventSystem;
class GameConfig;

class Pawn : public BaseObj, public ITickUpdatable
{
protected:
	Pawn(PawnProperty pawnProperty, const GameConfig& gameConfig, CollisionTags collision);

public:
	~Pawn() override;

	void Activate() override;
	void Deactivate() override;

	//BaseObj overrides
	void TakeDamage(unsigned int damage, Author author) override;
	[[nodiscard]] Author GetAuthor() const;

	void Heal(int amount);

	[[nodiscard]] Direction GetDirection() const;
	void SetDirection(Direction dir);

	[[nodiscard]] double GetSpeed() const;

protected:
	double _speed{};
	unsigned short _tier{1u};
	const std::vector<std::shared_ptr<BaseObj>>& _allObjects;
	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<IMoveBeh> _moveBeh{nullptr};
	Direction _dir{};
	Author _author{};
	const GameConfig& _gameConfig;

	std::vector<EventSubscription> _subs{};
	EventSubscription _tickUpdateSub{};

	virtual void Subscribe();
	void Unsubscribe();
	void SubscribeTickUpdate();
	void UnsubscribeTickUpdate();
	void OnTickUpdate(const TickUpdateEvent& event);
	virtual void OnDespawned(const DespawnedEvent& event);
	virtual void OnPosChanged(const PosChangedEvent& event);

	//TODO: implement collision detection through quadtree
	void TickUpdate(double deltaTime) override = 0;

private:
	virtual void SubscribeAsAuthority();
	virtual void SubscribeAsClient();
	void OnHealthChanged(const HealthChangedEvent& event);
};
