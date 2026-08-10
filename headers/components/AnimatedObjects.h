#pragma once

#include "../entities/BaseObj.h"

enum class Direction : char8_t;
enum class AnimationType : char8_t;

class AnimatedObject
{
public:
	ObjRectangle rect{};
	Direction dir{};
	int currentFrameIndex{};//NOTE: frame index currently shown, advances every animationSpeed ticks
	int ticksSinceLastFrame{};//NOTE: tick counter throttling how often currentFrameIndex advances
	int limitOfFrames{};
	int animationSpeed{};//NOTE: ticks between frame advances;
	AnimationType type{};
	bool markToDispose{};
	bool isInfinite{};
	int scale{};
	std::string name{};
	std::string nameWithUuid{};

	AnimatedObject(const AnimatedObject& other);
	AnimatedObject(AnimatedObject&& other) noexcept;

	AnimatedObject(const std::string& name, ObjRectangle rect, AnimationType type, int frameLimit, int scale,
				   int animationSpeed, bool isInfinite = {});

	~AnimatedObject();

	AnimatedObject& operator=(const AnimatedObject& other);
	AnimatedObject& operator=(AnimatedObject&& other) noexcept;
};
