#pragma once

#include <cstdint>

//NOTE: goes on the wire inside SignalEvent - append only, never reorder
enum class ClientSignal : std::uint8_t
{
	ReadyToPlay,
	RestartMatch,
};
