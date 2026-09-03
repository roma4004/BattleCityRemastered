#pragma once

#include "enums/ClientSignal.h"

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct SignalEvent final
{
	ClientSignal signal{};
};
}//namespace network::commands
