#pragma once

#include <string>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/uuid/uuid.hpp>

#include "UuidSerialization.h"
#include "../../enums/Direction.h"
#include "../commands/Command.h"

class TankShot : public Command
{
	friend class boost::serialization::access;

	std::string _who{};
	Direction _dir{};
	boost::uuids::uuid _uuid{};

public:
	//for deserialization
	TankShot();

	//for serialization
	TankShot(const std::string& who, Direction dir, boost::uuids::uuid uuid);

	~TankShot() override = default;

	const std::string& GetWho() const;
	Direction GetDir() const;
	boost::uuids::uuid GetUuid() const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/)
	{
		ar & boost::serialization::base_object<Command>(*this);
		ar & _who;
		ar & _dir;
		ar & _uuid;
	}

	const char* GetClassNameW() const override;
};

BOOST_CLASS_EXPORT_KEY(TankShot);
