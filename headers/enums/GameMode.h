#pragma once

enum class GameMode : char8_t
{
	Demo,

	OnePlayer,
	TwoPlayers,
	CoopWithBot,
	PlayAsHost,
	PlayAsClient,

	EndIterator// should be the last one
};

//NOTE: broader than IsHost - offline modes run the game too, they just replicate nothing
[[nodiscard]] constexpr bool IsAuthority(const GameMode mode) { return mode != GameMode::PlayAsClient; }

//NOTE: the other half of IsAuthority, spelled out so no call site has to read a negation
[[nodiscard]] constexpr bool IsClient(const GameMode mode) { return !IsAuthority(mode); }

[[nodiscard]] constexpr bool IsHost(const GameMode mode) { return mode == GameMode::PlayAsHost; }

//NOTE: the other axis - not who computes the game, but whether a wire exists at all
[[nodiscard]] constexpr bool IsNetworkGame(const GameMode mode)
{
	return mode == GameMode::PlayAsHost || mode == GameMode::PlayAsClient;
}

[[nodiscard]] constexpr bool IsLocalGame(const GameMode mode) { return !IsNetworkGame(mode); }

//NOTE: seat two is taken in every mode but one - by a bot in CoopWithBot/Demo, by the peer in a net game
[[nodiscard]] constexpr bool HasSecondPlayer(const GameMode mode) { return mode != GameMode::OnePlayer; }

//NOTE: Demo fills both seats with bots, CoopWithBot only the second one
[[nodiscard]] constexpr bool UsesCoopBots(const GameMode mode)
{
	return mode == GameMode::Demo || mode == GameMode::CoopWithBot;
}
