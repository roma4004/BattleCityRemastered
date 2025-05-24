#pragma once

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/uuid/uuid.hpp>

#include "Command.h"
#include "UuidSerialization.h"

enum TankType : char8_t;

class RespawnTank : public Command
{
	friend class boost::serialization::access;

	TankType _tankType{};
	boost::uuids::uuid _uuid{};

public:
	//for deserialization
	RespawnTank();

	//for serialization
	explicit RespawnTank(TankType tankType, boost::uuids::uuid uuid);

	~RespawnTank() override = default;

	TankType GetTankType() const;
	boost::uuids::uuid GetUuid() const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/)
	{
		ar & boost::serialization::base_object<Command>(*this);
		ar & _tankType;
		ar & _uuid;
	}

	const char* GetClassNameW() const override;
};

BOOST_CLASS_EXPORT_KEY(RespawnTank);
