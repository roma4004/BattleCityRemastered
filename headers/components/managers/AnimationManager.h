#pragma once

#include "../EventSystem.h"
#include "components/AnimatedObjects.h"
#include "enums/GameMode.h"
#include "network/commands/UuidSerialization.h"
#include <memory>
#include <vector>

enum class AnimationType: char8_t;
struct ObjRectangle;
class Tank;

class AnimationManager
{
	using buuid = boost::uuids::uuid;

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<AnimatedObject> _animatedObjects{};// all other (eg. explosion)
	std::vector<AnimatedObject> _tankObjects{};//procedural animated (eg tanks )
	std::vector<AnimatedObject> _waterObjects{};//idle animation
	std::string _name = "AnimationManager";
	GameMode _gameMode{};

public:
	explicit AnimationManager(const std::shared_ptr<EventSystem>& events);

	~AnimationManager();

	void Subscribe();
	void SubscribeAsHost();
	void SubscribeAsClient();

	void Unsubscribe() const;
	void UnsubscribeAsClient() const;
	void UnsubscribeAsHost() const;
	void SetGameMode(GameMode newGameMode);
	void Reset();
	void CreateAnimationWater(ObjRectangle rect);
	void CreateAnimationTank(const std::weak_ptr<Tank>& tank);
	void CreateAnimation(AnimationType type, ObjRectangle rect, const std::string& objName, int color);
	void DeleteTankAnimation(const std::string& objName);

private:
	void Create(const std::string& name, ObjRectangle rect, AnimationType type, int limitOfFrames,
	            int scale, std::string objName, int color);
	void Update();
	static void UpdateFrame(AnimatedObject& obj, int animationSpeed);
	static void UpdateFrameInfinite(AnimatedObject& obj, int animationSpeed);
	void UpdateTank(const std::string& objName);
	void DisableTankAnimation(const std::string& objName);
	void AnimationSeqDisposer();
};
