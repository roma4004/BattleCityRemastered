#pragma once

#include "enums/PlayerSlot.h"
#include <cstdint>

// Dispatch key for the input events. The slot half says which player a press belongs to, the origin
// half says which pipe it arrived on: a host has a keyboard producing presses for both slots, so
// without the origin its own arrow keys would be indistinguishable from the client's input off the
// wire. Both players are remote at once once the server becomes a process of its own.
enum class InputChannel : std::uint8_t
{
	LocalP1,
	LocalP2,
	RemoteP1,
	RemoteP2,
};

[[nodiscard]] constexpr InputChannel LocalInput(const PlayerSlot slot)
{
	return slot == PlayerSlot::P1 ? InputChannel::LocalP1 : InputChannel::LocalP2;
}

[[nodiscard]] constexpr InputChannel RemoteInput(const PlayerSlot slot)
{
	return slot == PlayerSlot::P1 ? InputChannel::RemoteP1 : InputChannel::RemoteP2;
}
