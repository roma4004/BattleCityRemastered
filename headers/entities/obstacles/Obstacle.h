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
struct DespawnedEvent;

class Obstacle : public BaseObj, public IDrawable
{
	void OnHealthChanged(const HealthChangedEvent& event);
	void OnDespawned(const DespawnedEvent&);
	void SubscribeAsClient();

protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	GameMode _gameMode{};
	ObstacleType _obstacleType{};

	void Draw() const override;

	virtual void EmitDeathStatistics(const std::string& author, const std::string& fraction) = 0;

	Obstacle(ObjRectangle rect, int health, std::string name, const std::shared_ptr<EventSystem>& events, Uuid uuid,
			 GameMode gameMode, ObstacleType obstacleType, CollisionTags collision);

public:
	~Obstacle() override;

	//BaseObj overrides
	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;
	void TakeDamage(unsigned int damage, const std::string& author, const std::string& fraction) override;
};
