#pragma once

#include "enums/CommandType.h"
#include "enums/DisconnectReason.h"

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct Disconnect final
{
	CommandType type{CommandType::DISCONNECT};
	DisconnectReason reason{};
};
}//namespace network::commands
