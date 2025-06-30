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
