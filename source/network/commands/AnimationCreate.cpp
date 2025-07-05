#include "network/commands/AnimationCreate.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>//NOTE: required for serialization uuid
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(AnimationCreate);

AnimationCreate::AnimationCreate(): Command(CommandType::ANIMATION_CREATE) {}

using buuid = boost::uuids::uuid;

AnimationCreate::AnimationCreate(const AnimationType type, const ObjRectangle rect, const buuid uuid)
	: Command(CommandType::ANIMATION_CREATE), _type(type), _rect(rect), _uuid(uuid) {}

AnimationType AnimationCreate::GetAnimationType() const { return _type; }
ObjRectangle AnimationCreate::GetRect() const { return _rect; }
buuid AnimationCreate::GetUuid() const { return _uuid; }

const char* AnimationCreate::GetClassNameW() const { return "AnimationCreate"; }
