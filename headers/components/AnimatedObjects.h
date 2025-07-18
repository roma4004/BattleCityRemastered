#pragma once

#include "../entities/BaseObj.h"
#include "interfaces/IDrawable.h"

#include <memory>

enum class AnimationType : char8_t;
enum class GameMode : char8_t;
class EventSystem;

class AnimatedObject : public BaseObj, public IDrawable
{
	using buuid = boost::uuids::uuid;

public:
	std::shared_ptr<EventSystem> events{nullptr};
	int animationFrame{0};
	int elapsedFrames{0};
	int limitOfFrames{};
	GameMode gameMode{};
	AnimationType type{};
	bool markToDispose{false};
	bool isInfinite{false};
	int scale{};
	std::string objName{};
	const BaseObj* parent{nullptr};

	void Draw(const BaseObj* obj) const override;
	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

	AnimatedObject(const AnimatedObject& other);
	AnimatedObject(AnimatedObject&& other) noexcept;

	AnimatedObject();

	//for water
	AnimatedObject(ObjRectangle rect, std::shared_ptr<EventSystem> events, int frameLimit);

	//for tank
	AnimatedObject(ObjRectangle rect, std::shared_ptr<EventSystem> events, buuid uuid, GameMode gameMode,
	               int frameLimit, int scale, std::string objName, const BaseObj* obj);

	//for other (eg explosion)
	AnimatedObject(std::string name, ObjRectangle rect, AnimationType type, std::shared_ptr<EventSystem> events,
	               buuid uuid, GameMode gameMode, int frameLimit, int scale, std::string objName);

	~AnimatedObject() override;

	void Subscribe();
	void Unsubscribe() const;

	void Disable() const;
	void Enable();

	AnimatedObject& operator=(const AnimatedObject& other);
	AnimatedObject& operator=(AnimatedObject&& other) noexcept;
};
