#pragma once

#include "Command.h"
#include "Point.h"
#include "UuidSerialization.h"
#include "enums/Direction.h"
#include <string>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/uuid/uuid.hpp>

class PositionChange : public Command
{
	using buuid = boost::uuids::uuid;

	friend class boost::serialization::access;

	std::string _who{};
	FPoint _pos{};
	buuid _uuid{};
	Direction _dir{};

public:
	//for deserialization
	PositionChange();

	//for serialization
	PositionChange(const std::string& who, const FPoint& pos, Direction dir, buuid uuid);

	~PositionChange() override = default;

	[[nodiscard]] const std::string& GetWho() const;
	[[nodiscard]] FPoint GetPos() const;
	[[nodiscard]] Direction GetDir() const;
	[[nodiscard]] buuid GetUuid() const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/);

	[[nodiscard]] const char* GetClassNameW() const override;
};

template<class Archive>
void PositionChange::serialize(Archive& ar, const unsigned int)
{
	ar & boost::serialization::base_object<Command>(*this);
	ar & _who;
	ar & _pos;
	ar & _dir;
	ar & _uuid;
}

BOOST_CLASS_EXPORT_KEY(PositionChange);
