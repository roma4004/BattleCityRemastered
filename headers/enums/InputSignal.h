#pragma once

#include <cstdint>

//NOTE: goes on the wire inside KeyStateChange - append only, never reorder.
//Mostly client-to-host input; PauseStatus is the one that travels the other way.
enum class InputSignal : std::uint8_t
{
	MoveUp,
	MoveDown,
	MoveLeft,
	MoveRight,
	Fire,
	PauseReleased,
	PauseStatus,
};
