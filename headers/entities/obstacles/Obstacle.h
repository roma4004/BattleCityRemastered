#pragma once

#include "../BaseObj.h"
#include "../Tags.h"
#include "components/EventSystem.h"
#include "interfaces/IDrawable.h"
#include <memory>
#include <vector>

enum class ObstacleType : char8_t;
enum class GameMode : char8_t;
class EventSystem;
struct HealthChangedEvent;

class Obstacle : public BaseObj, public IDrawable
{
	virtual void Subscribe();
	virtual void SubscribeAsClient();
	void OnHealthChanged(const HealthChangedEvent& event);

protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	// Shared with derived classes (BrickWall/SteelWall/BushTile/EagleTile/IceTile all override
	// Subscribe() and push into this same inherited vector) - one destruction point cleans up
	// both the base's and the derived class's subscriptions together, replacing the old two-stage
	// manual Unsubscribe() (derived override removes its own, then Obstacle::Unsubscribe() swept
	// the rest via RemoveAllListeners).
	std::vector<EventSubscription> _subs{};
	GameMode _gameMode{};
	ObstacleType _obstacleType{};

	void Draw() const override;

	virtual void EmitDeathStatistics(const std::string& author, const std::string& fraction) = 0;

public:
	Obstacle(ObjRectangle rect, int health, std::string name, const std::shared_ptr<EventSystem>& events, Uuid uuid,
			 GameMode gameMode, ObstacleType obstacleType, CollisionTags collision);

	~Obstacle() override;

	//BaseObj overrides
	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;
	void TakeDamage(unsigned int damage, const std::string& author, const std::string& fraction) override;
};
