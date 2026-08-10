#include "components/AnimatedObjects.h"
#include "enums/AnimationType.h"
#include "utils/UuidUtils.h"
#include <boost/uuid/nil_generator.hpp>

AnimatedObject::AnimatedObject(const std::string& name, const ObjRectangle rect, const AnimationType type,
							   const int frameLimit, const int scale, const bool isInfinite)
	: rect{rect}
	, limitOfFrames{frameLimit}
	, type(type)
	, isInfinite{isInfinite}
	, scale{scale}
	, name{name}
	, nameWithUuid{name + UuidUtils::GetStringUuid(UuidUtils::GetRandomUuid())} {}


AnimatedObject::~AnimatedObject() = default;

// copy constructor
AnimatedObject::AnimatedObject(const AnimatedObject& other)
{
	rect = other.rect;
	dir = other.dir;
	animationFrame = other.animationFrame;
	elapsedFrames = other.elapsedFrames;
	limitOfFrames = other.limitOfFrames;
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
	animationFrame = other.animationFrame;
	elapsedFrames = other.elapsedFrames;
	limitOfFrames = other.limitOfFrames;
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
		return *this;

	rect = other.rect;
	dir = other.dir;
	animationFrame = other.animationFrame;
	elapsedFrames = other.elapsedFrames;
	limitOfFrames = other.limitOfFrames;
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
		return *this;

	rect = other.rect;
	dir = other.dir;
	animationFrame = other.animationFrame;
	elapsedFrames = other.elapsedFrames;
	limitOfFrames = other.limitOfFrames;
	type = other.type;
	markToDispose = other.markToDispose;
	isInfinite = other.isInfinite;
	scale = other.scale;
	name = std::move(other.name);
	nameWithUuid = std::move(other.nameWithUuid);

	return *this;
}
