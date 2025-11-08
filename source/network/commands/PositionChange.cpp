#include "network/commands/PositionChange.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(PositionChange);

PositionChange::PositionChange() : Command{CommandType::POSITION_CHANGE} {}

PositionChange::PositionChange(std::string who, const FPoint pos, const Direction dir, buuid uuid)
	: Command{CommandType::POSITION_CHANGE}, _who{std::move(who)}, _pos{pos}, _dir{dir}, _uuid{uuid} {}

std::string PositionChange::GetWho() const noexcept { return _who; }

FPoint PositionChange::GetPos() const noexcept { return _pos; }

Direction PositionChange::GetDir() const noexcept { return _dir; }

using buuid = boost::uuids::uuid;
buuid PositionChange::GetUuid() const noexcept { return _uuid; }

const char* PositionChange::GetClassNameW() const noexcept { return "PositionChange"; }
