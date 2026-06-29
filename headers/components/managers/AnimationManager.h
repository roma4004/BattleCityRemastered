#pragma once

#include "../EventSystem.h"
#include "components/AnimatedObjects.h"
#include "enums/GameMode.h"
#include "network/commands/UuidSerialization.h"
#include <memory>
#include <vector>

enum class AnimationType : char8_t;
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
	// void SubscribeAsClient();

	void Unsubscribe() const;
	void SetGameMode(GameMode newGameMode);
	void Reset();
	void CreateAnimationWater(ObjRectangle rect);
	void CreateHelmetAnimation(ObjRectangle rect, const std::string& name);
	void CreateAnimationTank(ObjRectangle rect, std::string name);
	void CreateAnimation(AnimationType type, ObjRectangle rect, const std::string& name);
	void DeleteTankAnimation(const std::string& name);

private:
	void Create(const std::string& name, ObjRectangle rect, AnimationType type, int limitOfFrames, int scale,
				bool isInfinite = {});
	void Update();
	static void UpdateFrame(AnimatedObject& object, int animationSpeed);
	static void UpdateWaterFrame(AnimatedObject& object, int animationSpeed);
	void UpdateTank(const std::string_view& name, const ObjRectangle& rect, const Direction& dir);
	void UpdateHelmetEffect(const std::string_view& name, const ObjRectangle& rect);
	void DeleteHelmetAnimation(const std::string& name);
	void OnHelmetEffect(const std::string& name, bool isEnable);
	// void DisableTankAnimation(const std::string& name);
	void AnimationSeqDisposer();
	void DrawObject(const AnimatedObject& object) const;
	void Draw() const;
};
