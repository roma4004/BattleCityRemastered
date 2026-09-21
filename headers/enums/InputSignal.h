#pragma once

#include <cstdint>

//NOTE: goes on the wire inside KeyStateChange - append only, never reorder; only PauseStatus goes host to client
enum class InputSignal : std::uint8_t
{
	MoveUp,
	MoveDown,
	MoveLeft,
	MoveRight,
	Fire,
	PauseRequest,
	PauseStatus,
};
