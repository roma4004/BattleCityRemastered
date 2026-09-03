#pragma once

#include <cstdint>

//NOTE: goes on the wire inside Disconnect - append only, never reorder
enum class DisconnectReason : std::uint8_t
{
	PlayerQuit,
	HostShutdown,
	GameOver,

	ProtocolError,
};
