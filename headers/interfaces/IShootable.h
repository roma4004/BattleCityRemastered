#pragma once

#include "utils/Uuid.h"

class IShootable
{
protected:
	virtual ~IShootable() = default;

public:
	[[nodiscard]] virtual Uuid Shot(Uuid uuid = {}) = 0;
};
