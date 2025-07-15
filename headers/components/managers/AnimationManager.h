#pragma once

#include "../EventSystem.h"
#include "components/AnimatedObjects.h"
#include "enums/GameMode.h"
#include "network/commands/UuidSerialization.h"
#include <memory>
#include <vector>

enum class AnimationType: char8_t;
struct ObjRectangle;

class AnimationManager
{
	using buuid = boost::uuids::uuid;

	std::shared_ptr<EventSystem> _events{nullptr};
	AnimatedObject _waterAnimationPassport{16};
	std::vector<AnimatedObject> _animatedObjects;
	std::string _name = "AnimationManager";
	GameMode _gameMode{};

public:
	explicit AnimationManager(std::shared_ptr<EventSystem> events);

	~AnimationManager();

	void Subscribe();
	void SubscribeAsHost();
	void SubscribeAsClient();

	void Unsubscribe() const;
	void UnsubscribeAsClient() const;
	void UnsubscribeAsHost() const;

	[[nodiscard]] int GetFrame(buuid uuid, AnimationType type) const;
	[[nodiscard]] int GetWaterFrame() const;

private:
	void Create(const std::string& name, AnimationType type, ObjRectangle rect, const buuid& uuid, int limitOfFrames);
	void Update();
	static void UpdateFrame(AnimatedObject& obj, int animationSpeed);
	void UpdateTank(const buuid& uuid);
	void UpdateWaterAnimation();
	void AnimationSeqDisposer();
};
