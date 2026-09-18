#pragma once

#include "enums/ClientSignal.h"

namespace network::commands
{
struct SignalEvent final
{
	ClientSignal signal{};
};
}//namespace network::commands
