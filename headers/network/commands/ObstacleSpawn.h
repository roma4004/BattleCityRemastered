#pragma once

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/uuid/uuid.hpp>

#include "Command.h"
#include "UuidSerialization.h"
#include "../../Point.h"

enum ObstacleType : char8_t;

class ObstacleSpawn : public Command
{
	friend class boost::serialization::access;

	FPoint _pos{};
	ObstacleType _obstacleType{};
	boost::uuids::uuid _uuid{};

public:
	//for deserialization
	ObstacleSpawn();

	//for serialization
	ObstacleSpawn(const FPoint& pos, ObstacleType obstacleType, boost::uuids::uuid uuid);

	~ObstacleSpawn() override = default;

	FPoint GetPos() const;
	ObstacleType GetObstacleType() const;
	boost::uuids::uuid GetUuid() const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/)
	{
		ar & boost::serialization::base_object<Command>(*this);
		ar & _pos;
		ar & _obstacleType;
		ar & _uuid;
	}

	const char* GetClassNameW() const override;
};

BOOST_CLASS_EXPORT_KEY(ObstacleSpawn);
