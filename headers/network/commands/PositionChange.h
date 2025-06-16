#pragma once

#include <string>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/uuid/uuid.hpp>

#include "Command.h"
#include "UuidSerialization.h"
#include "../../Point.h"
#include "../../enums/Direction.h"


class PositionChange : public Command
{
	friend class boost::serialization::access;

	std::string _who{};
	FPoint _pos{};
	Direction _dir{};
	boost::uuids::uuid _uuid{};

public:
	//for deserialization
	PositionChange();

	//for serialization
	PositionChange(const std::string& who, const FPoint& pos, Direction dir, boost::uuids::uuid uuid);

	~PositionChange() override = default;

	const std::string& GetWho() const;
	FPoint GetPos() const;
	Direction GetDir() const;
	boost::uuids::uuid GetUuid() const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/)
	{
		ar & boost::serialization::base_object<Command>(*this);
		ar & _who;
		ar & _pos;
		ar & _dir;
		ar & _uuid;
	}

	const char* GetClassNameW() const override;
};

BOOST_CLASS_EXPORT_KEY(PositionChange);
