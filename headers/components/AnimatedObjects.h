#pragma once

#include "../entities/BaseObj.h"

enum class Direction : char8_t;
enum class AnimationType : char8_t;

class AnimatedObject
{
public:
	ObjRectangle rect{};
	Direction dir{};
	int animationFrame{};
	int elapsedFrames{};//TODO: Animation frame and the elapsed frame looks the same logically
	int limitOfFrames{};
	AnimationType type{};
	bool markToDispose{};
	bool isInfinite{};
	int scale{};
	std::string name{};
	std::string nameWithUuid{};

	AnimatedObject(const AnimatedObject& other);
	AnimatedObject(AnimatedObject&& other) noexcept;

	AnimatedObject(const std::string& name, ObjRectangle rect, AnimationType type, int frameLimit, int scale,
				   bool isInfinite = {});

	AnimatedObject(const std::string& name, ObjRectangle rect, AnimationType type, int frameLimit, int scale,
				   bool isInfinite, const std::string& nameWithUuid);

	~AnimatedObject();

	AnimatedObject& operator=(const AnimatedObject& other);
	AnimatedObject& operator=(AnimatedObject&& other) noexcept;
};
