#include "components/AnimatedObjects.h"
#include "components/EventSystem.h"
#include "entities/obstacles/Obstacle.h"
#include "enums/AnimationType.h"
#include "enums/GameMode.h"
#include "utils/UuidUtils.h"

#include <boost/uuid/nil_generator.hpp>

AnimatedObject::AnimatedObject()//TODO: remove this
	: BaseObj{{}, 0x0, 1, UuidUtils::GetRandomUuid(), "Water", "Neutral"} {}

//NOTE: used only for water
AnimatedObject::AnimatedObject(const ObjRectangle rect, std::shared_ptr<EventSystem> events, const int frameLimit)
	: BaseObj{rect, 0x0, 1, UuidUtils::GetRandomUuid(), "Water", "Neutral"},
	  events(std::move(events)),
	  limitOfFrames{frameLimit},
	  type(AnimationType::Water_Animation),
	  isInfinite{true},
	  scale{1},
	  objName{"Water"}
{
	Subscribe();
}

AnimatedObject::AnimatedObject(const ObjRectangle rect, std::shared_ptr<EventSystem> events, const buuid uuid,
                               const GameMode gameMode, const int frameLimit, const int scale,
                               std::string objName, const BaseObj* obj)
	: BaseObj{rect, 0x0, 1, uuid, "TankAnimation", "Neutral"},
	  events(std::move(events)),
	  limitOfFrames{frameLimit},
	  gameMode{gameMode},
	  type(AnimationType::Tank_Animation),
	  isInfinite{true},
	  scale{scale},
	  objName(std::move(objName)),
	  parent{obj}
{
	Subscribe();
}

using buuid = boost::uuids::uuid;

AnimatedObject::AnimatedObject(std::string name, const ObjRectangle rect, const AnimationType type,
                               std::shared_ptr<EventSystem> events, const buuid uuid, const GameMode gameMode,
                               const int frameLimit, const int scale, std::string objName)
	: BaseObj{rect, 0x0, 1, uuid, std::move(name), "Neutral"},
	  events(std::move(events)),
	  limitOfFrames{frameLimit},
	  gameMode{gameMode},
	  type(type),//TODO:remove field
	  scale{scale},
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
		this->events->EmitEvent("ServerSend_AnimationCreate", type, _rect, _uuid);
	}

	this->events->AddListener("Draw", _nameWithUuid, [this]() { this->Draw(this); });
}

void AnimatedObject::Unsubscribe() const
{
	events->RemoveListener("Draw", _nameWithUuid);
}

void AnimatedObject::Draw(const BaseObj* obj) const
{
	if (type == AnimationType::Water_Animation)
	{
		events->EmitEvent("DrawAnimation", obj, -animationFrame, scale, objName);
	}
	else if (type == AnimationType::Spawn_Animation)
	{
		events->EmitEvent("DrawAnimation", obj, animationFrame, scale, _name);
	}
	else if (type == AnimationType::Bullet_Animation)
	{
		events->EmitEvent("DrawAnimation", obj, animationFrame, scale, _name);
	}
	else if (type == AnimationType::Bullet_Explosion)
	{
		events->EmitEvent("DrawAnimation", obj, animationFrame, scale, _name);
	}
	else if (parent == nullptr)
	{
		events->EmitEvent("DrawAnimation", obj, animationFrame, scale, objName);
	}
	else
	{//for tanks
		events->EmitEvent("DrawAnimation", parent, animationFrame, scale, objName);
	}
}

void AnimatedObject::SendDamageStatistics(const std::string& /*author*/, const std::string& /*fraction*/) {}

void AnimatedObject::Disable() const { Unsubscribe(); };
void AnimatedObject::Enable() { Subscribe(); };

// copy constructor
AnimatedObject::AnimatedObject(const AnimatedObject& other)
	: BaseObj(other)
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
	// 	events->EmitEvent("ServerSend_AnimationCreate", type, _rect, _uuid);
	// }

	Subscribe();
}

// move constructor
AnimatedObject::AnimatedObject(AnimatedObject&& other) noexcept
	: BaseObj(std::move(other))
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

	BaseObj::operator=(other);

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
	// 	events->EmitEvent("ServerSend_AnimationCreate", type, _rect, _uuid);
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

	BaseObj::operator=(other);

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

	return *this;
}
