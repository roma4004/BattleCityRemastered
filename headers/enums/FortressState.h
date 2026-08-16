#pragma once

#include <cstdint>

//NOTE: goes on the wire inside FortressChange - append only, never reorder
enum class FortressState : std::uint8_t
{
	Died,
	ToBrick,
	ToSteel,
};
