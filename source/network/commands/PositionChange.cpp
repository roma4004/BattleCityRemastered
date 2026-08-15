#include "network/commands/PositionChange.h"

namespace network::commands
{
PositionChange::PositionChange(std::string who, const FPoint pos, const Direction dir, const buuid uuid)
	: _who{std::move(who)}
	, _pos{pos}
	, _dir{dir}
	, _uuid{uuid} {}

CommandType PositionChange::GetType() const noexcept { return _type; }

std::string PositionChange::GetWho() const noexcept { return _who; }

FPoint PositionChange::GetPos() const noexcept { return _pos; }

Direction PositionChange::GetDir() const noexcept { return _dir; }

using buuid = boost::uuids::uuid;
buuid PositionChange::GetUuid() const noexcept { return _uuid; }

const char* PositionChange::GetClassNameW() const noexcept { return "PositionChange"; }
}//namespace network::commands
