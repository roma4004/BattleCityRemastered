#include "network/commands/TankSpawnComplete.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>//NOTE: required for serialization uuid
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(network::commands::TankSpawnComplete)

namespace network::commands
{
using buuid = boost::uuids::uuid;

TankSpawnComplete::TankSpawnComplete()
	: Command{CommandType::TANK_SPAWN_COMPLETE} {}

TankSpawnComplete::TankSpawnComplete(const buuid uuid)
	: Command{CommandType::TANK_SPAWN_COMPLETE}
	, _uuid{uuid} {}

buuid TankSpawnComplete::GetUuid() const noexcept { return _uuid; }

const char* TankSpawnComplete::GetClassNameW() const noexcept { return "TankSpawnComplete"; }
}//namespace network::commands
