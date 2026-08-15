#pragma once

#include "UuidSerialization.h"
#include "entities/ObjRectangle.h"
#include "enums/CommandType.h"
#include <ser20/types/common.hpp>
#include <boost/uuid/uuid.hpp>

enum class ObstacleType : char8_t;

namespace network::commands
{
class ObstacleSpawn
{
	using buuid = boost::uuids::uuid;

	CommandType _type{CommandType::OBSTACLE_SPAWN};
	ObjRectangle _rect{};
	ObstacleType _obstacleType{};
	buuid _uuid{};

public:
	//for deserialization
	ObstacleSpawn() = default;

	//for serialization
	ObstacleSpawn(ObjRectangle rect, ObstacleType obstacleType, buuid uuid);

	[[nodiscard]] CommandType GetType() const noexcept;
	[[nodiscard]] ObjRectangle GetRect() const noexcept;
	[[nodiscard]] ObstacleType GetObstacleType() const noexcept;
	[[nodiscard]] buuid GetUuid() const noexcept;
	[[nodiscard]] const char* GetClassNameW() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);
};

template<class Archive>
void ObstacleSpawn::serialize(Archive& ar, const unsigned int)
{
	ar & _type;
	ar & _rect;
	ar & _obstacleType;
	ar & _uuid;
}
}//namespace network::commands
