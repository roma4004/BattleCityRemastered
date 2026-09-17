#pragma once

#include <cstdint>

//NOTE: what is happening, whatever mode fills the seats; the scoreboard is a view of Won and Over
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
