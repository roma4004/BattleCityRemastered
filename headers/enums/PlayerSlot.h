#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

enum class PlayerSlot : std::uint8_t
{
	P1,
	P2,
	P3,
	P4,
};

inline constexpr std::array kSlots{PlayerSlot::P1, PlayerSlot::P2, PlayerSlot::P3, PlayerSlot::P4};

//NOTE: the most seats a match can have
inline constexpr std::size_t kSeatCount{kSlots.size()};
//NOTE: unless the host says otherwise
inline constexpr std::size_t kDefaultSeats{2u};

//NOTE: who drives a seat of a network match - an empty one waits for a player to join
enum class SeatHolder : char8_t
{
	Empty,
	Player,
	Bot
};

[[nodiscard]] constexpr std::size_t SeatIndex(const PlayerSlot slot) noexcept { return static_cast<std::size_t>(slot); }

//NOTE: the devices pair up - Tab swaps the first two seats, Shift+Tab the other two
[[nodiscard]] constexpr PlayerSlot SlotForDevice(const std::size_t index, const bool isFirstPairSwapped,
												 const bool isSecondPairSwapped) noexcept
{
	const std::size_t seat{std::min(index, kSeatCount - 1u)};
	const bool isSwapped{seat < 2u ? isFirstPairSwapped : isSecondPairSwapped};

	return kSlots[isSwapped ? seat ^ 1u : seat];
}

[[nodiscard]] constexpr std::string_view ToString(const PlayerSlot slot) noexcept
{
	constexpr std::array kNames{std::string_view{"p1"}, std::string_view{"p2"}, std::string_view{"p3"},
								std::string_view{"p4"}};

	return kNames[SeatIndex(slot)];
}
