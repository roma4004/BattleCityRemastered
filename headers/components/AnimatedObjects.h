#pragma once

#include "../entities/BaseObj.h"
#include <memory>

enum class Direction : char8_t;
enum class AnimationType : char8_t;
enum class GameMode : char8_t;
class EventSystem;
class Tank;

class AnimatedObject
{
public:
	std::shared_ptr<EventSystem> events{nullptr};
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

	void Draw() const;

	AnimatedObject(const AnimatedObject& other);
	AnimatedObject(AnimatedObject&& other) noexcept;

	AnimatedObject(const std::string& name, ObjRectangle rect, AnimationType type,
				   const std::shared_ptr<EventSystem>& events, int frameLimit, int scale, bool isInfinite = {});

	~AnimatedObject();

	void Subscribe() const;
	void Unsubscribe() const;

	void Disable() const;
	void Enable() const;

	AnimatedObject& operator=(const AnimatedObject& other);
	AnimatedObject& operator=(AnimatedObject&& other) noexcept;
};
