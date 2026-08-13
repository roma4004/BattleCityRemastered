#pragma once

#include "../EventSystem.h"
#include "components/AnimatedObjects.h"
#include "components/events/AnimationRenderEvents.h"
#include "enums/AnimationType.h"
#include <memory>
#include <vector>

enum class AnimationType : char8_t;
struct ObjRectangle;
struct FPoint;
struct GameResetEvent;
struct PostTickUpdateEvent;
struct DrawEvent;
struct AnimationCreateTankMoveEvent;
struct AnimationCreateTankExplosionEvent;
struct AnimationCreateBulletExplosionEvent;
struct AnimationCreateWaterEvent;
struct AnimationTankUpdateEvent;
struct AnimationBonusHelmetChangeEvent;
class Tank;

class AnimationManager
{
public:
	explicit AnimationManager(const std::shared_ptr<EventSystem>& events);

	~AnimationManager() = default;

private:
	void Subscribe();

	void OnGameReset(const GameResetEvent&);
	void OnPostTickUpdate(const PostTickUpdateEvent&);
	void OnDraw(const DrawEvent&) const;
	void OnCreateTankSpawn(const AnimationCreateTankSpawnEvent& event);
	void OnCreateTankMove(const AnimationCreateTankMoveEvent& event);
	void OnCreateTankExplosion(const AnimationCreateTankExplosionEvent& event);
	void OnCreateBulletExplosion(const AnimationCreateBulletExplosionEvent& event);
	void OnCreateWaterFlow(const AnimationCreateWaterEvent& event);
	void OnUpdateTankMove(const AnimationTankUpdateEvent& event);
	void OnHelmetEffect(const AnimationBonusHelmetChangeEvent& event);

	void Reset();
	void DrawObject(const AnimatedObject& object) const;

	void Create(const std::string& name, ObjRectangle rect, AnimationType type, int size, int scale,
				int speed, bool isInfinite = {});
	void CreateAnimation(AnimationType type, ObjRectangle rect, const std::string& name);

	static void UpdateFrame(AnimatedObject& object);
	void OnHelmetEffect(const std::string& name, bool isEnable);
	void UpdateHelmetEffect(const std::string& name, const FPoint& pos);

	void DisableTankAnimation(const std::string& name);
	void DisableHelmetEffect(const std::string& name);

	static AnimatedObject* FindReusable(std::vector<AnimatedObject>& container, AnimationType type);

	struct AnimationPreset
	{
		std::string_view name{};
		int size{};
		int scale{};
		int speed{};
		bool isInfinite{};
	};

	struct KeyValue
	{
		AnimationType type{};
		AnimationPreset preset{};
	};

	static constexpr AnimationPreset GetPreset(AnimationType type);


	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	std::vector<AnimatedObject> _autoAnimatedObjects{};//advanced on TickUpdate() (eg. explosions, spawn, helmet)
	std::vector<AnimatedObject> _turnBasedTankObjects{};//advanced on movement (eg. tank move event)
	std::vector<AnimatedObject> _autoAnimatedWaterObjects{};//advanced on TickUpdate(), separated to render water first
};
