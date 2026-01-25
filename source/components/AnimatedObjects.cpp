#include "components/AnimatedObjects.h"
#include "components/EventSystem.h"
#include "entities/obstacles/Obstacle.h"
#include "entities/pawns/Tank.h"
#include "enums/AnimationType.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "utils/UuidUtils.h"
#include <boost/uuid/nil_generator.hpp>

class Tank;

using buuid = boost::uuids::uuid;

AnimatedObject::AnimatedObject(const std::string& name, const ObjRectangle rect, const AnimationType type,
                               const std::shared_ptr<EventSystem>& events, const GameMode gameMode,
                               const int frameLimit, const int scale, std::string objName, const int color,
                               const bool isInfinite, const std::weak_ptr<Tank> tank)
	: events(events),
	  rect{rect},
	  limitOfFrames{frameLimit},
	  color{color},
	  gameMode{gameMode},
	  type(type),
	  isInfinite{isInfinite},
	  scale{scale},
	  name{name},
	  nameWithUuid{name + UuidUtils::GetStringUuid(UuidUtils::GetRandomUuid())},
	  objName(std::move(objName)),
	  parent{tank}
{
	Subscribe();
}

AnimatedObject::~AnimatedObject()
{
	if (events != nullptr)
	{
		Unsubscribe();
	}
}

void AnimatedObject::Subscribe() const
{
	this->events->AddListener("Draw", nameWithUuid, [this]() { this->Draw(); });
}

void AnimatedObject::Unsubscribe() const
{
	if (events != nullptr)
	{
		events->RemoveListener("Draw", nameWithUuid);
	}
}

void AnimatedObject::Draw() const
{
	if (type == AnimationType::Water_Animation)
	{
		events->EmitEvent("DrawAnimation", rect, Direction::UP, -animationFrame, scale, name, color);
		//TODO: -animationFrame -> +animationFrame
	}
	else if (parent.expired())
	{
		if (events != nullptr)
		{
			events->EmitEvent("DrawAnimation", rect, Direction::UP, animationFrame, scale, name, color);
		}
	}
	else
	{
		//for tanks
		if (const auto tankLck = parent.lock())
		{
			if (events != nullptr)
			{
				events->EmitEvent("DrawTankAnimation", tankLck->GetRect(), tankLck->GetDirection(),
				                  animationFrame, scale, objName, tankLck->GetColor());
			}
		}
	}
}

void AnimatedObject::Disable() const { Unsubscribe(); };
void AnimatedObject::Enable() { Subscribe(); };

// copy constructor
AnimatedObject::AnimatedObject(const AnimatedObject& other)
{
	Disable();

	events = other.events;

	rect = other.rect;
	animationFrame = other.animationFrame;
	elapsedFrames = other.elapsedFrames;
	limitOfFrames = other.limitOfFrames;
	color = other.color;
	gameMode = other.gameMode;
	type = other.type;
	markToDispose = other.markToDispose;
	isInfinite = other.isInfinite;
	scale = other.scale;
	name = other.name;
	nameWithUuid = other.nameWithUuid;
	objName = other.objName;
	parent = other.parent;

	Enable();
}

// move constructor
AnimatedObject::AnimatedObject(AnimatedObject&& other) noexcept
{
	other.Disable();
	Disable();

	events = other.events;
	other.events = nullptr;

	rect = other.rect;
	animationFrame = other.animationFrame;
	elapsedFrames = other.elapsedFrames;
	limitOfFrames = other.limitOfFrames;
	color = other.color;
	gameMode = other.gameMode;
	type = other.type;
	markToDispose = other.markToDispose;
	isInfinite = other.isInfinite;
	scale = other.scale;
	name = std::move(other.name);
	nameWithUuid = std::move(other.nameWithUuid);
	objName = std::move(other.objName);
	parent = other.parent;

	Enable();
}

// copy assignment
AnimatedObject& AnimatedObject::operator=(const AnimatedObject& other)
{
	if (this == &other)
		return *this;

	Disable();

	events = other.events;

	rect = other.rect;
	animationFrame = other.animationFrame;
	elapsedFrames = other.elapsedFrames;
	limitOfFrames = other.limitOfFrames;
	color = other.color;
	gameMode = other.gameMode;
	type = other.type;
	markToDispose = other.markToDispose;
	isInfinite = other.isInfinite;
	scale = other.scale;
	name = other.name;
	nameWithUuid = other.nameWithUuid;
	objName = other.objName;
	parent = other.parent;

	Enable();

	return *this;
}

//TODO: recheck all move flow and dispose flow
// move assignment
AnimatedObject& AnimatedObject::operator=(AnimatedObject&& other) noexcept
{
	if (this == &other)
		return *this;

	other.Disable();
	Disable();

	events = other.events;
	other.events = nullptr;

	rect = other.rect;
	animationFrame = other.animationFrame;
	elapsedFrames = other.elapsedFrames;
	limitOfFrames = other.limitOfFrames;
	color = other.color;
	gameMode = other.gameMode;
	type = other.type;
	markToDispose = other.markToDispose;
	isInfinite = other.isInfinite;
	scale = other.scale;
	name = std::move(other.name);
	nameWithUuid = std::move(other.nameWithUuid);
	objName = std::move(other.objName);
	parent = other.parent;

	Enable();

	return *this;
}
