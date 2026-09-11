#pragma once

#include <cstddef>
#include <cstdint>

enum class PlayerSlot : std::uint8_t
{
	P1,
	P2,
};

//NOTE: the left keyboard half and the first pad are both device zero
[[nodiscard]] constexpr PlayerSlot SlotForDevice(const std::size_t index, const bool areSwapped)
{
	return (index == 0u) != areSwapped ? PlayerSlot::P1 : PlayerSlot::P2;
}
