#pragma once

#include "../PlayerKeys.h"

class IInputProvider
{
public:
	[[nodiscard]] PlayerKeys GetKeysStats() const { return playerKeys; }

protected:
	PlayerKeys playerKeys;
};
