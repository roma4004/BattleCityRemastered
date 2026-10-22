#pragma once

#include <cstdint>

//NOTE: a seat whose player left a running match, while the rest are asked about it; on the wire, append only
enum class Absence : std::uint8_t
{
	None,
	//NOTE: the match waits for this one while nobody answers
	Left,
	//NOTE: the one who left took the seat again, and the match goes on once somebody says so
	Back
};

//NOTE: what a player still in answers - the first answer decides
enum class AbsenceChoice : std::uint8_t
{
	//NOTE: without the ones who left - their tanks leave the field, their seats wait for them
	Continue,
	//NOTE: with bots driving the tanks of the ones who left, until they take them back
	Bot
};
