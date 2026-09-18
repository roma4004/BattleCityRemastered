#pragma once

#include "enums/InputSignal.h"

namespace network::commands
{
struct KeyStateChange final
{
	InputSignal action{};
	bool isPressed{};
};
}//namespace network::commands
