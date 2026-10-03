#pragma once

//NOTE: who fights whom in a network match; GameMode says who fills the seats
enum class MatchRules : char8_t
{
	//NOTE: the players defend the base together
	Classic,
	FreeForAll
};
