#pragma once

#include "../EventSystem.h"
#include "components/AnimatedObjects.h"
#include "components/events/AnimationRenderEvents.h"
#include "enums/AnimationType.h"
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
struct AnimationCreateTankMoveEvent;
struct AnimationCreateTankExplosionEvent;
struct AnimationCreateBulletExplosionEvent;
struct AnimationCreateWaterEvent;
struct AnimationTankUpdateEvent;
struct AnimationBonusHelmetChangeEvent;

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
	void OnCreateTankMove(const AnimationCreateTankMoveEvent& event);
	void OnCreateTankExplosion(const AnimationCreateTankExplosionEvent& event);
	void OnCreateBulletExplosion(const AnimationCreateBulletExplosionEvent& event);
	void OnCreateWaterFlow(const AnimationCreateWaterEvent& event);
	void OnUpdateTankMove(const AnimationTankUpdateEvent& event);
	void OnHelmetEffect(const AnimationBonusHelmetChangeEvent& event);

	void Reset();
	void DrawObject(const AnimatedObject& object) const;

	void Create(Author author, ObjRectangle rect, AnimationType type, int size, int scale,
				int speed, int passes, Uuid owner);
	void CreateAnimation(AnimationType type, ObjRectangle rect, Author author, Uuid owner = {});

	static bool UpdateFrame(AnimatedObject& object);
	void OnHelmetEffect(Author author, bool isEnable);
	void UpdateHelmetEffect(Author author, const FPoint& pos);

	void DisableTankAnimation(Author author);
	void DisableHelmetEffect(Author author);

	static AnimatedObject* FindReusable(std::vector<AnimatedObject>& container, AnimationType type);

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
	bool _isPaused{false};
	std::vector<EventSubscription> _subs{};
	std::vector<AnimatedObject> _autoAnimatedObjects{};//advanced on TickUpdate() (eg. explosions, spawn, helmet)
	std::vector<AnimatedObject> _turnBasedTankObjects{};//advanced on movement (eg. tank move event)
	std::vector<AnimatedObject> _autoAnimatedWaterObjects{};//advanced on TickUpdate(), separated to render water first
};
