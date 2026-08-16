#include "components/AnimatedObjects.h"
#include "enums/AnimationType.h"
#include "utils/UuidUtils.h"
#include <boost/uuid/nil_generator.hpp>

AnimatedObject::AnimatedObject(const std::string& objName, const ObjRectangle objRect, const AnimationType objType,
							   const int frameLimit, const int objScale, const int animationSpeed, const bool infinite)
	: rect{objRect}
	, size{frameLimit}
	, speed{animationSpeed}
	, type(objType)
	, isInfinite{infinite}
	, scale{objScale}
	, name{objName}
	, nameWithUuid{objName + UuidUtils::GetStringUuid(UuidUtils::GetRandomUuid())} {}


AnimatedObject::~AnimatedObject() = default;

// copy constructor
AnimatedObject::AnimatedObject(const AnimatedObject& other)
{
	rect = other.rect;
	dir = other.dir;
	currentFrameIndex = other.currentFrameIndex;
	ticksSinceLastFrame = other.ticksSinceLastFrame;
	size = other.size;
	speed = other.speed;
	type = other.type;
	markToDispose = other.markToDispose;
	isInfinite = other.isInfinite;
	scale = other.scale;
	name = other.name;
	nameWithUuid = other.nameWithUuid;
}

// move constructor
AnimatedObject::AnimatedObject(AnimatedObject&& other) noexcept
{
	rect = other.rect;
	dir = other.dir;
	currentFrameIndex = other.currentFrameIndex;
	ticksSinceLastFrame = other.ticksSinceLastFrame;
	size = other.size;
	speed = other.speed;
	type = other.type;
	markToDispose = other.markToDispose;
	isInfinite = other.isInfinite;
	scale = other.scale;
	name = std::move(other.name);
	nameWithUuid = std::move(other.nameWithUuid);
}

// copy assignment
AnimatedObject& AnimatedObject::operator=(const AnimatedObject& other)
{
	if (this == &other)
	{
		return *this;
	}

	rect = other.rect;
	dir = other.dir;
	currentFrameIndex = other.currentFrameIndex;
	ticksSinceLastFrame = other.ticksSinceLastFrame;
	size = other.size;
	speed = other.speed;
	type = other.type;
	markToDispose = other.markToDispose;
	isInfinite = other.isInfinite;
	scale = other.scale;
	name = other.name;
	nameWithUuid = other.nameWithUuid;

	return *this;
}

// move assignment
AnimatedObject& AnimatedObject::operator=(AnimatedObject&& other) noexcept
{
	if (this == &other)
	{
		return *this;
	}

	rect = other.rect;
	dir = other.dir;
	currentFrameIndex = other.currentFrameIndex;
	ticksSinceLastFrame = other.ticksSinceLastFrame;
	size = other.size;
	speed = other.speed;
	type = other.type;
	markToDispose = other.markToDispose;
	isInfinite = other.isInfinite;
	scale = other.scale;
	name = std::move(other.name);
	nameWithUuid = std::move(other.nameWithUuid);

	return *this;
}
