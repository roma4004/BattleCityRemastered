#pragma once

#include "../BaseObj.h"
#include "../Tags.h"
#include "components/EventSystem.h"
#include "interfaces/IDrawable.h"
#include <memory>
#include <vector>

enum class Faction : char8_t;
enum class ObstacleType : char8_t;
struct HealthChangedEvent;
struct DespawnedEvent;
class EventSystem;
class GameConfig;

class Obstacle : public BaseObj, public IDrawable
{
	void OnHealthChanged(const HealthChangedEvent& event);
	void SubscribeAsClient();

protected:
	//NOTE: empty, not pure - WaterTile draws nothing of its own and has nothing to add here
	virtual void Subscribe();
	virtual void OnDespawned(const DespawnedEvent& event);
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	const GameConfig& _gameConfig;
	ObstacleType _obstacleType{};

	void Draw() const override;

	void EmitDeathStatistics(Author author) override = 0;

	Obstacle(ObjRectangle rect, int health, const std::shared_ptr<EventSystem>& events, Uuid uuid,
			 const GameConfig& gameConfig, ObstacleType obstacleType, CollisionTags collision);

public:
	void Activate() override;
	void Deactivate() override;

	void TakeDamage(unsigned int damage, Author author) override;
};
