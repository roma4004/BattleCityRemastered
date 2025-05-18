#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include "../../../headers/network/commands/PositionChange.h"

BOOST_CLASS_EXPORT_IMPLEMENT(PositionChange);

PositionChange::PositionChange(): Command(CommandType::POSITION_CHANGE) {}

PositionChange::PositionChange(const std::string& who, const FPoint& pos, const Direction dir,
                               const boost::uuids::uuid uuid)
	: Command(CommandType::POSITION_CHANGE), _who(who), _pos(pos), _dir(dir), _uuid(uuid) {}

const std::string& PositionChange::GetWho() const { return _who; }

FPoint PositionChange::GetPos() const { return _pos; }

Direction PositionChange::GetDir() const { return _dir; }

boost::uuids::uuid PositionChange::GetUuid() const { return _uuid; }

const char* PositionChange::GetClassNameW() const { return "PositionChange"; }
