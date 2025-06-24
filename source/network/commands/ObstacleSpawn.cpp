#include "network/commands/ObstacleSpawn.h"
#include "entities/ObjRectangle.h"
#include "enums/ObstacleType.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(ObstacleSpawn);

ObstacleSpawn::ObstacleSpawn(): Command(CommandType::OBSTACLE_SPAWN) {}

ObstacleSpawn::ObstacleSpawn(const ObjRectangle& rect, const ObstacleType obstacleType, const buuid uuid)
	: Command(CommandType::OBSTACLE_SPAWN), _rect(rect), _uuid(uuid), _obstacleType(obstacleType) {}

ObjRectangle ObstacleSpawn::GetRect() const { return _rect; }

ObstacleType ObstacleSpawn::GetObstacleType() const { return _obstacleType; }

using buuid = boost::uuids::uuid;
buuid ObstacleSpawn::GetUuid() const { return _uuid; }

const char* ObstacleSpawn::GetClassNameW() const { return "ObstacleSpawn"; }
