#pragma once

#include "enums/InputSignal.h"

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct KeyStateChange final
{
	InputSignal action{};
	bool isPressed{};
};
}//namespace network::commands
