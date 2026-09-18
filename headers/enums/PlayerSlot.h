#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

enum class PlayerSlot : std::uint8_t
{
	P1,
	P2,
};

//NOTE: the left keyboard half and the first pad are both device zero
[[nodiscard]] constexpr PlayerSlot SlotForDevice(const std::size_t index, const bool areSwapped) noexcept
{
	return (index == 0u) != areSwapped ? PlayerSlot::P1 : PlayerSlot::P2;
}

[[nodiscard]] constexpr std::string_view ToString(const PlayerSlot slot) noexcept
{
	return slot == PlayerSlot::P1 ? "p1" : "p2";
}
