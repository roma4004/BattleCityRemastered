#pragma once

#include <cstdint>

//NOTE: how a command crosses the wire - every command type names its own as kDelivery
enum class Delivery : std::uint8_t
{
	//NOTE: changes the field for good - retransmitted until acknowledged, handed over in the order sent
	Reliable,
	//NOTE: overwritten by the next value for the same entity - only the newest one is ever sent again
	Latest,
};
