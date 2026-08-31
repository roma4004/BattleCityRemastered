#pragma once

#include <cstdint>

//NOTE: the phase, not the mode - a lobby happens *as* a host or *as* a client, a demo runs *in*
//whatever mode fills the seats. No Score: the scoreboard is a view of Won/Over
enum class GameState : char8_t
{
	Menu,
	Demo,
	Lobby,
	Playing,
	Paused,
	Won,
	Over
};
