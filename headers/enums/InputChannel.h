#pragma once

#include "enums/PlayerSlot.h"
#include <cstdint>

// Dispatch key for the input events: the slot half says whose press it is, the origin half which pipe
// it came in on. One machine can hold a keyboard for both slots and take wire input for the same ones
enum class InputChannel : std::uint8_t
{
	LocalP1,
	LocalP2,
	LocalP3,
	LocalP4,
	RemoteP1,
	RemoteP2,
	RemoteP3,
	RemoteP4,
};

[[nodiscard]] constexpr InputChannel LocalInput(const PlayerSlot slot) noexcept
{
	return static_cast<InputChannel>(SeatIndex(slot));
}

[[nodiscard]] constexpr InputChannel RemoteInput(const PlayerSlot slot) noexcept
{
	return static_cast<InputChannel>(kSeatCount + SeatIndex(slot));
}
