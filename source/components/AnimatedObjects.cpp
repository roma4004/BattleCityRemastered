#include "components/AnimatedObjects.h"
#include "components/EventSystem.h"
#include "entities/obstacles/Obstacle.h"
#include "enums/AnimationType.h"
#include "enums/GameMode.h"

#include <boost/uuid/nil_generator.hpp>

using buuid = boost::uuids::uuid;

AnimatedObject::AnimatedObject()
	: BaseObj{{}, 0x0, 1, {}, "Water", "Neutral"} {}

AnimatedObject::AnimatedObject(const int frameLimit)
	: BaseObj{{}, 0x0, 1, {}, "Water", "Neutral"},
	  limitOfFrames{frameLimit},
	  type(AnimationType::Water_Animation)
{
	if (gameMode == GameMode::PlayAsHost)
	{
		this->events->EmitEvent("ServerSend_AnimationCreate", type, _rect, _uuid);
	}
}

AnimatedObject::AnimatedObject(std::string name, const ObjRectangle rect, const AnimationType type,
                               std::shared_ptr<EventSystem> events, const buuid uuid, const GameMode gameMode,
                               const int frameLimit)
	: BaseObj{rect, 0x0, 1, uuid, std::move(name), "Neutral"},
	  events(std::move(events)),
	  limitOfFrames{frameLimit},
	  gameMode{gameMode},
	  type(type)
{
	if (gameMode == GameMode::PlayAsHost)
	{
		this->events->EmitEvent("ServerSend_AnimationCreate", this->type, _rect, _uuid);
	}

	this->events->AddListener("Draw", _nameWithUuid, [this]() { this->Draw(this); });
}

AnimatedObject::~AnimatedObject()
{
	events->RemoveListener("Draw", _nameWithUuid);
};

void AnimatedObject::Draw(const BaseObj* obj) const
{
	events->EmitEvent("DrawObj", obj);
}

void AnimatedObject::SendDamageStatistics(const std::string& /*author*/, const std::string& /*fraction*/) {}
