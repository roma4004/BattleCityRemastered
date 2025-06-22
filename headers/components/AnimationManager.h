#pragma once

#include "EventSystem.h"
#include <memory>
#include <vector>

class AnimationManager
{
	struct AnimationStruct//TODO make normal name
	{
		std::string name;
		int animationFrame{};
		int currentFrameCounter{};
		int animationIdLimit{};
	};

	AnimationStruct animWater{"Water", 0, 0, 16};
	AnimationStruct animExplosion{"Explosion", 0, 0, 5};
	std::vector<AnimationStruct> _animatedObj;
	std::string _name = "AnimationManager";
	std::shared_ptr<EventSystem> _events{nullptr};

public:
	AnimationManager(std::shared_ptr<EventSystem> events);

	~AnimationManager();
	
	void Subscribe();
	void Unsubscribe() const;

	[[nodiscard]] int GetAnimFrame(const std::string& name) const;
	[[nodiscard]] int GetAnimWater();
	[[nodiscard]] int GetAnimExplosion();
};
