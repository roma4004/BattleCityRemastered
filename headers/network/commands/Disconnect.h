#pragma once

#include "enums/DisconnectReason.h"

namespace network::commands
{
struct Disconnect final
{
	DisconnectReason reason{};
};
}//namespace network::commands
