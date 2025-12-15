#include "network/commands/AnimationCreate.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>//NOTE: required for serialization uuid
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(AnimationCreate);

AnimationCreate::AnimationCreate() : Command{CommandType::ANIMATION_CREATE} {}

using buuid = boost::uuids::uuid;

AnimationCreate::AnimationCreate(const AnimationType type, const ObjRectangle rect, std::string name, const int color)
	: Command{CommandType::ANIMATION_CREATE}, _type{type}, _rect{rect}, _name{std::move(name)}, _color{color} {}

AnimationType AnimationCreate::GetAnimationType() const noexcept { return _type; }

ObjRectangle AnimationCreate::GetRect() const noexcept { return _rect; }

std::string AnimationCreate::GetName() const noexcept { return _name; }

int AnimationCreate::GetColor() const noexcept { return _color; }

const char* AnimationCreate::GetClassNameW() const noexcept { return "AnimationCreate"; }
