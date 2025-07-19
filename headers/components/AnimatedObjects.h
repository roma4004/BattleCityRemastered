#pragma once

#include "../entities/BaseObj.h"
#include "enums/Direction.h"
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
	int animationFrame{0};
	int elapsedFrames{0};
	int limitOfFrames{};
	int color{};
	GameMode gameMode{};
	Direction dir{};
	AnimationType type{};
	bool markToDispose{false};
	bool isInfinite{false};
	int scale{};
	std::string name{};
	std::string nameWithUuid{};
	std::string objName{};
	std::weak_ptr<Tank> parent{};

	void Draw() const;

	AnimatedObject(const AnimatedObject& other);
	AnimatedObject(AnimatedObject&& other) noexcept;

	//for water
	AnimatedObject(ObjRectangle rect, std::shared_ptr<EventSystem> events, int frameLimit);

	//for tank
	AnimatedObject(std::shared_ptr<EventSystem> events, GameMode gameMode, int frameLimit, int scale,
	               std::weak_ptr<Tank> tank);

	//for other (eg explosion)
	AnimatedObject(const std::string& name, ObjRectangle rect, AnimationType type, std::shared_ptr<EventSystem> events,
	               GameMode gameMode, int frameLimit, int scale, std::string objName, int color);

	~AnimatedObject();

	void Subscribe();
	void Unsubscribe() const;

	void Disable() const;
	void Enable();

	AnimatedObject& operator=(const AnimatedObject& other);
	AnimatedObject& operator=(AnimatedObject&& other) noexcept;
};
