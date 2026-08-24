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
struct HealthChangedEvent;
struct DespawnedEvent;
struct TickUpdateEvent;

class Pawn : public BaseObj, public ITickUpdatable
{
protected:
	Pawn(PawnProperty pawnProperty, const GameConfig& gameConfig, CollisionTags collision);

public:
	~Pawn() override;

	//BaseObj overrides
	void TakeDamage(unsigned int damage, const std::string& author, const std::string& fraction) override;

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
	const GameConfig& _gameConfig;

	std::vector<EventSubscription> _subs{};
	EventSubscription _tickUpdateSub{};

	virtual void Subscribe();
	void Unsubscribe();
	void SubscribeTickUpdate();
	void UnsubscribeTickUpdate();
	void OnTickUpdate(const TickUpdateEvent& event);
	virtual void OnDespawned(const DespawnedEvent& event);

	//TODO: implement collision detection through quadtree
	void TickUpdate(double deltaTime) override = 0;

private:
	virtual void SubscribeAsAuthority();
	virtual void SubscribeAsClient();
	void OnHealthChanged(const HealthChangedEvent& event);
};
