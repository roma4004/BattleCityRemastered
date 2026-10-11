#pragma once

#include "../EventSystem.h"
#include "components/AnimatedObjects.h"
#include "components/events/AnimationRenderEvents.h"
#include "enums/AnimationType.h"
#include "utils/Uuid.h"
#include <memory>
#include <span>
#include <vector>

enum class AnimationType : char8_t;
enum class DrawLayer : char8_t;
struct ObjRectangle;
struct GameResetEvent;
struct PostTickUpdateEvent;
struct PauseStatusEvent;
struct AnimationCreateBonusSpawnEvent;
struct AnimationCancelTankSpawnEvent;
struct AnimationMoveTankSpawnEvent;
struct TankSpawnCompletedEvent;
struct BonusSpawnCompletedEvent;
struct AnimationCreateTankExplosionEvent;
struct AnimationCreateBulletExplosionEvent;
struct AnimationCreateWaterEvent;

//NOTE: what only a spawn burst carries - the uuid it reports to, and whether it is a client's endless one
struct AnimationExtras final
{
	Uuid owner{};
	bool isEndless{};
};

class AnimationManager final
{
public:
	explicit AnimationManager(const std::shared_ptr<EventSystem>& events);

	//NOTE: disposed ones included - a finished slot waits here to be reused
	[[nodiscard]] std::span<const AnimatedObject> OnLayer(DrawLayer layer) const;

private:
	void Subscribe();

	void OnGameReset(const GameResetEvent&);
	void OnPostTickUpdate(const PostTickUpdateEvent&);
	void OnPauseStatus(const PauseStatusEvent& event);
	void OnCreateTankSpawn(const AnimationCreateTankSpawnEvent& event);
	void OnCreateBonusSpawn(const AnimationCreateBonusSpawnEvent& event);
	void OnCancelTankSpawn(const AnimationCancelTankSpawnEvent& event);
	void OnMoveTankSpawn(const AnimationMoveTankSpawnEvent& event);
	void OnTankSpawnCompleted(const TankSpawnCompletedEvent& event);
	void OnBonusSpawnCompleted(const BonusSpawnCompletedEvent& event);
	void OnCreateTankExplosion(const AnimationCreateTankExplosionEvent& event);
	void OnCreateBulletExplosion(const AnimationCreateBulletExplosionEvent& event);
	void OnCreateWaterFlow(const AnimationCreateWaterEvent& event);

	void Reset();

	void Place(const AnimatedObject& animation);
	[[nodiscard]] std::vector<AnimatedObject>& ContainerOf(AnimationType type);
	void CreateAnimation(AnimationType type, ObjRectangle rect, const AnimationExtras& extras = {});

	void Cancel(AnimationType type, Uuid owner);

	struct AnimationPreset
	{
		int size{};
		int scale{};
		int speed{};
		int passes{1};
	};

	struct KeyValue
	{
		AnimationType type{};
		AnimationPreset preset{};
	};

	static constexpr AnimationPreset GetPreset(AnimationType type);


	std::shared_ptr<EventSystem> _events{nullptr};
	bool _isPaused{};
	std::vector<EventSubscription> _subs{};
	//NOTE: the container is the layer - the water is driven over, the bursts flash over the field
	std::vector<AnimatedObject> _groundAnimations{};
	std::vector<AnimatedObject> _effectAnimations{};
};
