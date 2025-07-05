#include "animations/AnimatedObjects.h"
#include "entities/obstacles/Obstacle.h"
#include "components/EventSystem.h"
#include "enums/AnimationType.h"
#include "enums/GameMode.h"

#include <boost/uuid/nil_generator.hpp>

using buuid = boost::uuids::uuid;

AnimatedObject::AnimatedObject(std::string name, ObjRectangle rect, AnimationType type,
                               std::shared_ptr<EventSystem> events,
                               buuid uuid, GameMode gameMode, int frameLimit)
	: BaseObj{rect, 0x0, 1, uuid, std::move(name), "Neutral"},
	  events(std::move(events)),
	  limitOfFrames{frameLimit},
	  gameMode{gameMode},
	  type(type)
{
	if (gameMode == GameMode::PlayAsHost)
	{
		this->events->EmitEvent<const AnimationType, const ObjRectangle&, const buuid&>(
				"ServerSend_AnimationCreate", type, _rect, uuid);
	}
}

AnimatedObject::AnimatedObject(int frameLimit)
	: BaseObj{{}, 0x0, 1, {}, "Water", "Neutral"},
	  limitOfFrames{frameLimit},
	  type(AnimationType::Water_Animation)
{
	if (gameMode == GameMode::PlayAsHost)
	{
		this->events->EmitEvent<const AnimationType, const ObjRectangle&, const buuid&>(
				"ServerSend_AnimationCreate", type, _rect, {});
	}
}

AnimatedObject::AnimatedObject(): BaseObj{{}, 0x0, 1, {}, "Water", "Neutral"} {}

AnimatedObject::~AnimatedObject() = default;

void AnimatedObject::Draw(const BaseObj* obj) const
{
	//events->EmitEvent<const BaseObj*>("DrawObj", obj);
}

void AnimatedObject::SendDamageStatistics(const std::string& author, const std::string& fraction) {}
