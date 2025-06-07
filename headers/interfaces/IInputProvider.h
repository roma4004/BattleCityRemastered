#pragma once

#include "../PlayerKeys.h"

class IInputProvider
{
protected:
	PlayerKeys playerKeys;

public:
	[[nodiscard]] PlayerKeys GetKeysStats() const { return playerKeys; }
};
