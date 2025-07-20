#include "components/AnimatedObjects.h"
#include "components/EventSystem.h"
#include "entities/obstacles/Obstacle.h"
#include "entities/pawns/Tank.h"
#include "enums/AnimationType.h"
#include "enums/GameMode.h"
#include "utils/UuidUtils.h"
#include <boost/uuid/nil_generator.hpp>

class Tank;

//NOTE: used only for water
AnimatedObject::AnimatedObject(const ObjRectangle rect, std::shared_ptr<EventSystem> events, const int frameLimit)
	: events(std::move(events)),
	  rect{rect},
	  limitOfFrames{frameLimit},
	  type(AnimationType::Water_Animation),
	  isInfinite{true},
	  scale{1},
	  name{"Water"},
	  nameWithUuid{"name" + UuidUtils::GetStringUuid(UuidUtils::GetRandomUuid())},
	  objName{"Water"}
{
	Subscribe();
}

AnimatedObject::AnimatedObject(std::shared_ptr<EventSystem> events, const GameMode gameMode, const int frameLimit,
                               const int scale, const std::weak_ptr<Tank>& tank)
	: events(std::move(events)),
	  limitOfFrames{frameLimit},
	  gameMode{gameMode},
	  type(AnimationType::Tank_Animation),
	  isInfinite{true},
	  scale{scale},
	  name(std::string("TankAnimation")),
	  nameWithUuid{"TankAnimation" + UuidUtils::GetStringUuid(UuidUtils::GetRandomUuid())},
	  parent{tank}
{
	const auto tankLck = tank.lock();
	rect = tankLck->GetRect();
	objName = std::string(tankLck->GetName());
	color = tankLck->GetColor();
	Subscribe();
}

using buuid = boost::uuids::uuid;

AnimatedObject::AnimatedObject(const std::string& name, const ObjRectangle rect, const AnimationType type,
                               std::shared_ptr<EventSystem> events, const GameMode gameMode, const int frameLimit,
                               const int scale, std::string objName, const int color)
	: events(std::move(events)),
	  rect{rect},
	  limitOfFrames{frameLimit},
	  color{color},
	  gameMode{gameMode},//TODO:remove field
	  type(type),
	  scale{scale},
	  name{name},
	  nameWithUuid{name + UuidUtils::GetStringUuid(UuidUtils::GetRandomUuid())},
	  objName(std::move(objName))
{
	Subscribe();
}

AnimatedObject::~AnimatedObject()
{
	Unsubscribe();
}

void AnimatedObject::Subscribe()
{
	if (gameMode == GameMode::PlayAsHost)
	{
		this->events->EmitEvent("ServerSend_AnimationCreate", type, rect, objName, color);
	}

	this->events->AddListener("Draw", nameWithUuid, [this]() { this->Draw(); });
}

void AnimatedObject::Unsubscribe() const
{
	events->RemoveListener("Draw", nameWithUuid);
}

void AnimatedObject::Draw() const
{
	if (type == AnimationType::Water_Animation)
	{
		events->EmitEvent("DrawAnimation", rect, dir, -animationFrame, scale, name, color);
	}
	else if (parent.expired())
	{
		events->EmitEvent("DrawAnimation", rect, dir, animationFrame, scale, name, color);
		//TODO: recheck if it needed?
	}
	else
	{
		//for tanks
		const std::shared_ptr<Tank> tankLck = parent.lock();
		events->EmitEvent("DrawAnimation", tankLck->GetRect(), tankLck->GetDirection(),
		                  animationFrame, scale, objName, tankLck->GetColor());
		//TODO: recheck if it needed?
	}
}

void AnimatedObject::Disable() const { Unsubscribe(); };
void AnimatedObject::Enable() { Subscribe(); };

// copy constructor
AnimatedObject::AnimatedObject(const AnimatedObject& other)
{
	events = other.events;
	animationFrame = other.animationFrame;
	elapsedFrames = other.elapsedFrames;
	limitOfFrames = other.limitOfFrames;
	gameMode = other.gameMode;
	type = other.type;
	markToDispose = other.markToDispose;
	isInfinite = other.isInfinite;
	scale = other.scale;
	objName = other.objName;
	parent = other.parent;
	// if (gameMode == GameMode::PlayAsHost) {
	// 	events->EmitEvent("ServerSend_AnimationCreate", type, rect, color);
	// }

	Subscribe();
}

// move constructor
AnimatedObject::AnimatedObject(AnimatedObject&& other) noexcept
{
	// Disable();

	events = other.events;
	// other.events = nullptr;

	animationFrame = other.animationFrame;
	elapsedFrames = other.elapsedFrames;
	limitOfFrames = other.limitOfFrames;
	gameMode = other.gameMode;
	type = other.type;
	markToDispose = other.markToDispose;
	isInfinite = other.isInfinite;
	scale = other.scale;
	objName = std::move(other.objName);
	parent = std::move(other.parent);

	Enable();
}

// copy assignment
AnimatedObject& AnimatedObject::operator=(const AnimatedObject& other)
{
	if (this == &other)
		return *this;

	// Disable();

	events = other.events;

	animationFrame = other.animationFrame;
	elapsedFrames = other.elapsedFrames;
	limitOfFrames = other.limitOfFrames;
	gameMode = other.gameMode;
	type = other.type;
	markToDispose = other.markToDispose;
	isInfinite = other.isInfinite;
	scale = other.scale;
	objName = other.objName;
	parent = other.parent;
	// if (gameMode == GameMode::PlayAsHost) {
	// 	events->EmitEvent("ServerSend_AnimationCreate", type, rect, color);
	// }

	Enable();

	return *this;
}


//TODO: recheck all move flow and dispose flow
// move assignment
AnimatedObject& AnimatedObject::operator=(AnimatedObject&& other) noexcept
{
	if (this == &other)
		return *this;

	// Disable();

	events = other.events;
	// other.events = nullptr;

	animationFrame = other.animationFrame;
	elapsedFrames = other.elapsedFrames;
	limitOfFrames = other.limitOfFrames;
	gameMode = other.gameMode;
	type = other.type;
	markToDispose = other.markToDispose;
	isInfinite = other.isInfinite;
	scale = other.scale;
	objName = other.objName;
	parent = other.parent;

	Enable();

	return *this;
}
