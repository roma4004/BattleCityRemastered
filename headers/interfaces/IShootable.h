#pragma once

#include "utils/Uuid.h"
#include <optional>

//NOTE: one shot as both sides have to see it - the name of the bullet and the damage it left with
struct ShotResult final
{
	Uuid uuid{};
	unsigned int damage{};
};

class IShootable
{
protected:
	virtual ~IShootable() = default;

public:
	//NOTE: empty uuid and damage are ours to mint and roll; a client always passes the host's, so both mean one shot
	[[nodiscard]] virtual ShotResult Shot(std::optional<Uuid> uuid, std::optional<unsigned int> damage) = 0;
};
