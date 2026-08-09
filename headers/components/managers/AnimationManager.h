#pragma once

#include "../EventSystem.h"
#include "components/AnimatedObjects.h"
#include "enums/GameMode.h"
#include "network/commands/UuidSerialization.h"
#include <memory>
#include <vector>

enum class AnimationType : char8_t;
struct ObjRectangle;
struct FPoint;
class Tank;

class AnimationManager
{
	using buuid = boost::uuids::uuid;

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	std::vector<AnimatedObject> _autoAnimatedObjects{};//advanced on TickUpdate() (eg. explosions, spawn, helmet)
	std::vector<AnimatedObject> _turnBasedTankObjects{};//advanced on movement (eg. tank move event)
	std::vector<AnimatedObject> _autoAnimatedWaterObjects{};//advanced on TickUpdate(), separated to render water first
	std::string _name = "AnimationManager";
	GameMode _gameMode{};

public:
	explicit AnimationManager(const std::shared_ptr<EventSystem>& events);

	~AnimationManager() = default;

	void Subscribe();
	void SubscribeAsHost();
	// void SubscribeAsClient();

	void SetGameMode(GameMode newGameMode);
	void Reset();
	void CreateAnimation(AnimationType type, ObjRectangle rect, const std::string& name);
	void DeleteTankAnimation(const std::string& name);

private:
	void Create(const std::string& name, ObjRectangle rect, AnimationType type, int limitOfFrames, int scale,
				int animationSpeed, bool isInfinite = {}, bool isLocallySimulated = {});
	void Update();
	static void UpdateFrame(AnimatedObject& object);
	void UpdateTank(const std::string& name, const FPoint& pos, const Direction& dir);
	void UpdateHelmetEffect(const std::string& name, const FPoint& pos);
	void DeleteHelmetAnimation(const std::string& name);
	void OnHelmetEffect(const std::string& name, bool isEnable);
	static AnimatedObject* FindReusable(std::vector<AnimatedObject>& container, AnimationType type);
	void DrawObject(const AnimatedObject& object) const;
	void Draw() const;
};
