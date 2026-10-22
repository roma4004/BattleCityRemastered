#pragma once

#include "enums/PlayerSlot.h"
#include <cstdint>

//NOTE: the input events' dispatch key - whose press it is (slot) and where it came from (local keys or the wire)
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
