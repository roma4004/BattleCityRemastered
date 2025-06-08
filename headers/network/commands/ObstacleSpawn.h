#pragma once

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/uuid/uuid.hpp>

#include "Command.h"
#include "UuidSerialization.h"
#include "../../ObjRectangle.h"

enum ObstacleType : char8_t;

class ObstacleSpawn : public Command
{
	using buuid = boost::uuids::uuid;

	friend class boost::serialization::access;

	ObjRectangle _rect{};
	ObstacleType _obstacleType{};
	buuid _uuid{};

public:
	//for deserialization
	ObstacleSpawn();

	//for serialization
	ObstacleSpawn(const ObjRectangle& rect, ObstacleType obstacleType, buuid uuid);

	~ObstacleSpawn() override = default;

	ObjRectangle GetRect() const;
	ObstacleType GetObstacleType() const;
	buuid GetUuid() const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/)
	{
		ar & boost::serialization::base_object<Command>(*this);
		ar & _rect;
		ar & _obstacleType;
		ar & _uuid;
	}

	const char* GetClassNameW() const override;
};

BOOST_CLASS_EXPORT_KEY(ObstacleSpawn);
