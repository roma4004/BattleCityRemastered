#pragma once

#include "enums/PlayerSlot.h"
#include <cstdint>

// Dispatch key for the input events: the slot half says whose press it is, the origin half which pipe
// it came in on. One machine can hold a keyboard for both slots and take wire input for the same ones
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
