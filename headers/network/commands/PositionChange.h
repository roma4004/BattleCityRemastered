#pragma once

#include "Command.h"
#include "UuidSerialization.h"
#include "../../Point.h"
#include "../../enums/Direction.h"
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
	Direction _dir{};
	buuid _uuid{};

public:
	//for deserialization
	PositionChange();

	//for serialization
	PositionChange(const std::string& who, const FPoint& pos, Direction dir, buuid uuid);

	~PositionChange() override = default;

	const std::string& GetWho() const;
	FPoint GetPos() const;
	Direction GetDir() const;
	buuid GetUuid() const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/);

	const char* GetClassNameW() const override;
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
