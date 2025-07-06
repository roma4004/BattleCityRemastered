#pragma once

#include "../entities/BaseObj.h"

#include <memory>

enum class AnimationType : char8_t;
enum class GameMode : char8_t;
class EventSystem;

class AnimatedObject : public BaseObj
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

	void Draw(const BaseObj* obj) const override;
	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

	AnimatedObject(std::string name, ObjRectangle rect, AnimationType type, std::shared_ptr<EventSystem> events,
	               buuid uuid, GameMode gameMode, int frameLimit);

	explicit AnimatedObject(int frameLimit);

	AnimatedObject();

	~AnimatedObject() override;
};
