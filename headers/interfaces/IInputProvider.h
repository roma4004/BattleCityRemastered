#pragma once

#include "components/input/PlayerKeys.h"

class IInputProvider
{
protected:
	PlayerKeys _playerKeys{};

public:
	[[nodiscard]] PlayerKeys GetKeysStats() const { return _playerKeys; }
};
