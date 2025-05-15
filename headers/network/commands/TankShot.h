#pragma once

#include <string>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>

#include "../../enums/Direction.h"
#include "../commands/Command.h"

class TankShot : public Command
{
	friend class boost::serialization::access;

	std::string _who{};
	Direction _dir{};

public:
	//for deserialization
	TankShot();

	//for serialization
	TankShot(const std::string& who, Direction dir);

	~TankShot() override = default;

	const std::string& GetWho() const;
	Direction GetDir() const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/)
	{
		ar & boost::serialization::base_object<Command>(*this);
		ar & _who;
		ar & _dir;
	}

	const char* GetClassNameW() const override;
};

BOOST_CLASS_EXPORT_KEY(TankShot);
