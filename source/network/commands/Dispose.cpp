#include "network/commands/Dispose.h"

namespace network::commands
{
Dispose::Dispose(std::string who, const buuid uuid)
	: _who{std::move(who)}
	, _uuid{uuid} {}

CommandType Dispose::GetType() const noexcept { return _type; }

std::string Dispose::GetWho() const noexcept { return _who; }

using buuid = boost::uuids::uuid;
buuid Dispose::GetUuid() const noexcept { return _uuid; }

const char* Dispose::GetClassNameW() const noexcept { return "Dispose"; }
}//namespace network::commands
