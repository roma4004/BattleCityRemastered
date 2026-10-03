#pragma once

#include <cstddef>

enum class GameMode : char8_t
{
	OnePlayer,
	TwoPlayers,
	CoopWithBot,
	FreeForAll,
	TwoPlayersFreeForAll,
	PlayAsHost,
	PlayAsClient,

	EndIterator
};

//NOTE: broader than IsHost - offline modes run the game too, they just replicate nothing
[[nodiscard]] constexpr bool IsAuthority(const GameMode mode) noexcept { return mode != GameMode::PlayAsClient; }

[[nodiscard]] constexpr bool IsClient(const GameMode mode) noexcept { return !IsAuthority(mode); }

[[nodiscard]] constexpr bool IsHost(const GameMode mode) noexcept { return mode == GameMode::PlayAsHost; }

//NOTE: the other axis - not who computes the game, but whether a wire exists at all
[[nodiscard]] constexpr bool IsNetworkGame(const GameMode mode) noexcept
{
	return mode == GameMode::PlayAsHost || mode == GameMode::PlayAsClient;
}

[[nodiscard]] constexpr bool IsLocalGame(const GameMode mode) noexcept { return !IsNetworkGame(mode); }

//NOTE: every tank for itself, no base
[[nodiscard]] constexpr bool IsFreeForAll(const GameMode mode) noexcept
{
	return mode == GameMode::FreeForAll || mode == GameMode::TwoPlayersFreeForAll;
}

//NOTE: seats a local mode fills; CoopWithBot's bot takes the second
[[nodiscard]] constexpr std::size_t LocalSeats(const GameMode mode) noexcept
{
	return mode == GameMode::OnePlayer || mode == GameMode::FreeForAll ? 1u : 2u;
}

//NOTE: seat one is the phase's call, not the mode's - the demo hands both seats to bots
[[nodiscard]] constexpr bool UsesCoopBots(const GameMode mode) noexcept { return mode == GameMode::CoopWithBot; }
