#pragma once

enum GameMode : int
{
	Demo,

	OnePlayer,
	TwoPlayers,
	CoopWithAI,
	PlayAsHost,
	PlayAsClient,

	EndIterator// should be the last one
};
