#pragma once

#include <cstdint>

//NOTE: the phase, not the mode - a lobby happens *as* a host or *as* a client. No Score: the
//scoreboard is a view of Won/Over
enum class GameState : char8_t
{
	Menu,
	Lobby,
	Playing,
	Paused,
	Won,
	Over
};
