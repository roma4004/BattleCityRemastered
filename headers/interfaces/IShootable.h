#pragma once

#include "utils/Uuid.h"
#include <optional>

class IShootable
{
protected:
	virtual ~IShootable() = default;

public:
	//NOTE: no uuid means the shot is ours to name; a client passes the host's so both sides
	//agree on which bullet this is
	[[nodiscard]] virtual Uuid Shot(std::optional<Uuid> uuid) = 0;
};
