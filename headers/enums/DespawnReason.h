#pragma once

enum class DespawnReason : char8_t
{
	None,
	Destroyed,
	PickedUp,
	Expired
};
