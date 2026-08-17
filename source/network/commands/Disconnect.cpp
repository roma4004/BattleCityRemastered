#include "network/commands/Disconnect.h"

namespace network::commands
{
Disconnect::Disconnect(const DisconnectReason reason)
	: _reason{reason} {}

CommandType Disconnect::GetType() const noexcept { return _type; }

DisconnectReason Disconnect::GetReason() const noexcept { return _reason; }

const char* Disconnect::GetClassNameW() const noexcept { return "Disconnect"; }
}//namespace network::commands
