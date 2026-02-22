#pragma once

#include "../entities/BaseObj.h"
#include <memory>

enum class AnimationType : char8_t;
enum class GameMode : char8_t;
class EventSystem;
class Tank;

class AnimatedObject
{
public:
	std::shared_ptr<EventSystem> events{nullptr};
	ObjRectangle rect{};
	int animationFrame{};
	int elapsedFrames{};
	int limitOfFrames{};
	unsigned int color{};
	GameMode gameMode{};
	AnimationType type{};
	bool markToDispose{};
	bool isInfinite{};
	int scale{};
	std::string name{};
	std::string nameWithUuid{};
	std::string objName{};
	std::weak_ptr<Tank> parent{};

	void Draw() const;

	AnimatedObject(const AnimatedObject& other);
	AnimatedObject(AnimatedObject&& other) noexcept;

	AnimatedObject(const std::string& name, ObjRectangle rect, AnimationType type,
				   const std::shared_ptr<EventSystem>& events, GameMode gameMode, int frameLimit, int scale,
				   std::string objName, unsigned int color, bool isInfinite = {}, std::weak_ptr<Tank> tank = {});

	~AnimatedObject();

	void Subscribe() const;
	void Unsubscribe() const;

	void Disable() const;
	void Enable();

	AnimatedObject& operator=(const AnimatedObject& other);
	AnimatedObject& operator=(AnimatedObject&& other) noexcept;
};
