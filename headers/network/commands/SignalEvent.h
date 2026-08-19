#pragma once

#include "enums/ClientSignal.h"
#include "enums/CommandType.h"

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct SignalEvent final
{
	CommandType type{CommandType::SIGNAL_EVENT};
	ClientSignal signal{};
};
}//namespace network::commands
