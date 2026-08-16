#pragma once

#include <cstdint>

//NOTE: goes on the wire inside KeyStateChange - append only, never reorder.
//None is for input that belongs to no player, e.g. pause.
enum class PlayerTag : std::uint8_t
{
	None,
	P1,
	P2,
};
