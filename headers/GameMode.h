#pragma once

enum GameMode : char8_t
{
	Demo,

	OnePlayer,
	TwoPlayers,
	CoopWithAI,
	PlayAsHost,
	PlayAsClient,

	EndIterator// should be the last one
};
