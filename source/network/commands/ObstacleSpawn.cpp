#include "../../../headers/network/commands/ObstacleSpawn.h"
#include "../../../headers/enums/ObstacleType.h"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(ObstacleSpawn);

ObstacleSpawn::ObstacleSpawn(): Command(CommandType::OBSTACLE_SPAWN) {}

ObstacleSpawn::ObstacleSpawn(const FPoint& pos, const ObstacleType obstacleType, const boost::uuids::uuid uuid)
	: Command(CommandType::OBSTACLE_SPAWN), _pos(pos), _obstacleType(obstacleType), _uuid(uuid) {}

FPoint ObstacleSpawn::GetPos() const { return _pos; }

ObstacleType ObstacleSpawn::GetObstacleType() const { return _obstacleType; }

boost::uuids::uuid ObstacleSpawn::GetUuid() const { return _uuid; }

const char* ObstacleSpawn::GetClassNameW() const { return "ObstacleSpawn"; }
