#pragma once

#include "enums/DisconnectReason.h"

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct Disconnect final
{
	DisconnectReason reason{};
};
}//namespace network::commands
