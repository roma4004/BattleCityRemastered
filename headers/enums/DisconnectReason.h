#pragma once

#include <cstdint>

//NOTE: goes on the wire inside Disconnect - append only, never reorder
enum class DisconnectReason : std::uint8_t
{
	PlayerQuit,
	HostShutdown,
	GameOver,
	//NOTE: a frame the peer could not deserialise - see CommandDispatcher::Dispatch
	ProtocolError,
};
