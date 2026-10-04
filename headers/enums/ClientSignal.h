#pragma once

#include <cstdint>

//NOTE: goes on the wire inside SignalEvent - append only, never reorder
enum class ClientSignal : std::uint8_t
{
	ReadyToPlay,
	RestartMatch,
	NextLevel,
	//NOTE: the snapshot is applied - a server holding the match for this client lets it go on
	WorldSynced,
	//NOTE: the answers to a player leaving the match, one per AbsenceChoice
	AbsenceContinue,
	AbsenceBot,
};
