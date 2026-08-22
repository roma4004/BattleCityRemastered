#pragma once

#include "components/input/PlayerKeys.h"

class IInputProvider
{
protected:
	PlayerKeys _playerKeys{};

public:
	virtual ~IInputProvider() = default;

	[[nodiscard]] PlayerKeys GetKeysStats() const { return _playerKeys; }

	virtual void Enable() = 0;
	virtual void Disable() = 0;
};
