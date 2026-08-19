#pragma once

#include "enums/CommandType.h"
#include "enums/InputSignal.h"
#include "enums/PlayerTag.h"

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct KeyStateChange final
{
	CommandType type{CommandType::KEY_STATE_CHANGE};
	PlayerTag tag{};
	InputSignal action{};
	bool isPressed{};
};
}//namespace network::commands
