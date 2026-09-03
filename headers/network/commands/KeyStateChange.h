#pragma once

#include "enums/InputSignal.h"
#include "enums/PlayerTag.h"

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct KeyStateChange final
{
	PlayerTag tag{};
	InputSignal action{};
	bool isPressed{};
};
}//namespace network::commands
