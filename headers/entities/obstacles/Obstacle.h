#pragma once

#include "../BaseObj.h"
#include "../Tags.h"
#include "components/EventSystem.h"
#include "components/Sprite.h"
#include "enums/DrawLayer.h"
#include <memory>
#include <optional>
#include <vector>

enum class Faction : char8_t;
enum class ObstacleType : char8_t;
struct HealthChangedEvent;
struct DespawnedEvent;
struct WorldSnapshotRequestedEvent;
class EventSystem;
class GameConfig;

class Obstacle : public BaseObj
{
	const DrawLayer _layer;

	void OnHealthChanged(const HealthChangedEvent& event);
	void OnWorldSnapshotRequested(const WorldSnapshotRequestedEvent& event) const;
	void SubscribeAsClient();

protected:
	//NOTE: empty, not pure - only the eagle has something to hear
	virtual void Subscribe();
	virtual void OnDespawned(const DespawnedEvent& event);
	ObstacleType _obstacleType{};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	const GameConfig& _gameConfig;

	void EmitDeathStatistics(Author author) override = 0;

	Obstacle(ObjRectangle rect, unsigned int health, const std::shared_ptr<EventSystem>& events, Uuid uuid,
			 const GameConfig& gameConfig, ObstacleType obstacleType, CollisionTags collision, DrawLayer layer);

public:
	//NOTE: one quarter of brick or steel - a shell spends this much health on each layer it penetrates
	static constexpr unsigned int kWallHealth{10};

	void Activate() override;
	void Deactivate() override;

	void TakeDamage(unsigned int damage, Author author) override;

	[[nodiscard]] std::optional<Sprite> Look() const override;
};
