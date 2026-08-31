#pragma once

#include "../BaseObj.h"
#include "../Tags.h"
#include "components/EventSystem.h"
#include "interfaces/IDrawable.h"
#include <memory>
#include <string>
#include <vector>

enum class Faction : char8_t;
enum class ObstacleType : char8_t;
enum class GameMode : char8_t;
class EventSystem;
struct HealthChangedEvent;
struct DespawnedEvent;

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
	GameMode _gameMode{};
	ObstacleType _obstacleType{};

	void Draw() const override;

	void EmitDeathStatistics(const std::string& author, Faction faction) override = 0;

	Obstacle(ObjRectangle rect, int health, std::string name, const std::shared_ptr<EventSystem>& events, Uuid uuid,
			 GameMode gameMode, ObstacleType obstacleType, CollisionTags collision);

public:
	~Obstacle() override;

	void Activate() override;
	void Deactivate() override;

	//BaseObj overrides
	void TakeDamage(unsigned int damage, const std::string& author, Faction faction) override;
};
