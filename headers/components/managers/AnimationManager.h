#pragma once

#include "../EventSystem.h"
#include "components/AnimatedObjects.h"
#include "components/events/AnimationRenderEvents.h"
#include "enums/AnimationType.h"
#include "enums/TankModel.h"
#include "utils/Uuid.h"
#include <memory>
#include <vector>

enum class AnimationType : char8_t;
struct ObjRectangle;
struct FPoint;
struct GameResetEvent;
struct PostTickUpdateEvent;
struct PauseStatusEvent;
struct DrawEvent;
struct PostDrawEvent;
struct AnimationCreateBonusSpawnEvent;
struct AnimationCancelTankSpawnEvent;
struct AnimationMoveTankSpawnEvent;
struct TankSpawnCompletedEvent;
struct BonusSpawnCompletedEvent;
struct AnimationCreateTankMoveEvent;
struct AnimationCreateTankExplosionEvent;
struct AnimationCreateBulletExplosionEvent;
struct AnimationCreateWaterEvent;
struct AnimationTankUpdateEvent;
struct AnimationBonusHelmetChangeEvent;

//NOTE: what only some animations carry - the uuid a burst reports to, a client's endless burst, a tank's look
struct AnimationExtras final
{
	Uuid owner{};
	bool isEndless{};
	TankModel model{};
	unsigned short tier{1u};
};

class AnimationManager final
{
public:
	explicit AnimationManager(const std::shared_ptr<EventSystem>& events);

private:
	void Subscribe();

	void OnGameReset(const GameResetEvent&);
	void OnPostTickUpdate(const PostTickUpdateEvent&);
	void OnPauseStatus(const PauseStatusEvent& event);
	void OnDraw(const DrawEvent&) const;
	void OnPostDraw(const PostDrawEvent&) const;
	void OnCreateTankSpawn(const AnimationCreateTankSpawnEvent& event);
	void OnCreateBonusSpawn(const AnimationCreateBonusSpawnEvent& event);
	void OnCancelTankSpawn(const AnimationCancelTankSpawnEvent& event);
	void OnMoveTankSpawn(const AnimationMoveTankSpawnEvent& event);
	void OnTankSpawnCompleted(const TankSpawnCompletedEvent& event);
	void OnBonusSpawnCompleted(const BonusSpawnCompletedEvent& event);
	void OnCreateTankMove(const AnimationCreateTankMoveEvent& event);
	void OnCreateTankExplosion(const AnimationCreateTankExplosionEvent& event);
	void OnCreateBulletExplosion(const AnimationCreateBulletExplosionEvent& event);
	void OnCreateWaterFlow(const AnimationCreateWaterEvent& event);
	void OnUpdateTankMove(const AnimationTankUpdateEvent& event);
	void OnHelmetEffect(const AnimationBonusHelmetChangeEvent& event);

	void Reset();
	void DrawObject(const AnimatedObject& object) const;

	void Place(const AnimatedObject& animation);
	[[nodiscard]] std::vector<AnimatedObject>& ContainerOf(AnimationType type);
	void CreateAnimation(AnimationType type, ObjRectangle rect, Author author, const AnimationExtras& extras = {});

	void OnHelmetEffect(Author author, bool isEnable);
	void UpdateHelmetEffect(Author author, const FPoint& pos);

	void Cancel(AnimationType type, Uuid owner);
	void DisableTankAnimation(Author author);
	void DisableHelmetEffect(Author author);

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
	std::vector<AnimatedObject> _autoAnimatedObjects{};//advanced on TickUpdate() (eg. explosions, spawn, helmet)
	std::vector<AnimatedObject> _turnBasedTankObjects{};//advanced on movement (eg. tank move event)
	std::vector<AnimatedObject> _autoAnimatedWaterObjects{};//advanced on TickUpdate(), separated to render water first
};
