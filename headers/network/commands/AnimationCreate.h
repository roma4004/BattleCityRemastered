#pragma once

#include "Command.h"
#include "UuidSerialization.h"
#include "entities/ObjRectangle.h"
#include "enums/AnimationType.h"
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/uuid/uuid.hpp>

class AnimationCreate : public Command
{
	using buuid = boost::uuids::uuid;

	friend class boost::serialization::access;

	AnimationType _type{};
	ObjRectangle _rect{};
	buuid _uuid{};

public:
	//for deserialization
	AnimationCreate();

	//for serialization
	explicit AnimationCreate(AnimationType type, ObjRectangle rect, buuid uuid);

	~AnimationCreate() override = default;

	[[nodiscard]] AnimationType GetAnimationType() const noexcept;
	[[nodiscard]] ObjRectangle GetRect() const noexcept;
	[[nodiscard]] buuid GetUuid() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/);

	[[nodiscard]] const char* GetClassNameW() const noexcept override;
};

template<class Archive>
void AnimationCreate::serialize(Archive& ar, const unsigned int)
{
	ar & boost::serialization::base_object<Command>(*this);
	ar & _type;
	ar & _rect;
	ar & _uuid;
}

BOOST_CLASS_EXPORT_KEY(AnimationCreate);
