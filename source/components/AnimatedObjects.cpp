#include "components/AnimatedObjects.h"
#include "components/EventSystem.h"
#include "entities/obstacles/Obstacle.h"
#include "enums/AnimationType.h"
#include "enums/Direction.h"
#include "utils/UuidUtils.h"
#include <boost/uuid/nil_generator.hpp>

AnimatedObject::AnimatedObject(const std::string& name, const ObjRectangle rect, const AnimationType type,
							   const std::shared_ptr<EventSystem>& events, const int frameLimit, const int scale,
							   std::string objName, const unsigned int color, const bool isInfinite)
	: events(events)
	, rect{rect}
	, limitOfFrames{frameLimit}
	, color{color}
	, type(type)
	, isInfinite{isInfinite}
	, scale{scale}
	, name{name}
	, nameWithUuid{name + UuidUtils::GetStringUuid(UuidUtils::GetRandomUuid())}
	, objName(std::move(objName))
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
	else if (name == "TankAnimation")
	{
		//for tanks
		if (events != nullptr)
		{
			events->EmitEvent("DrawTankAnimation", rect, dir, animationFrame, scale, objName, color);
		}
	}//TODO: how to merge this branches
	else
	{
		//bullets, explosions and other
		if (events != nullptr)
		{
			events->EmitEvent("DrawAnimation", rect, Direction::UP, animationFrame, scale, name, color);
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
	type = other.type;
	markToDispose = other.markToDispose;
	isInfinite = other.isInfinite;
	scale = other.scale;
	name = other.name;
	nameWithUuid = other.nameWithUuid;
	objName = other.objName;

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
	type = other.type;
	markToDispose = other.markToDispose;
	isInfinite = other.isInfinite;
	scale = other.scale;
	name = std::move(other.name);
	nameWithUuid = std::move(other.nameWithUuid);
	objName = std::move(other.objName);

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
	type = other.type;
	markToDispose = other.markToDispose;
	isInfinite = other.isInfinite;
	scale = other.scale;
	name = other.name;
	nameWithUuid = other.nameWithUuid;
	objName = other.objName;

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
	type = other.type;
	markToDispose = other.markToDispose;
	isInfinite = other.isInfinite;
	scale = other.scale;
	name = std::move(other.name);
	nameWithUuid = std::move(other.nameWithUuid);
	objName = std::move(other.objName);

	Enable();

	return *this;
}
