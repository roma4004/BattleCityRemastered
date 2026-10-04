#pragma once

#include "utils/Uuid.h"
#include <memory>
#include <vector>

class BaseObj;

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
	//NOTE: every shell the tier fires at once, named and rolled by us - the world says where the volley has to end
	[[nodiscard]] virtual std::vector<ShotResult> Volley(const std::vector<std::shared_ptr<BaseObj>>& objects) = 0;
	//NOTE: one shell of the host's volley, as the host named and rolled it - its next move puts it in its place
	virtual void Mirror(Uuid uuid, unsigned int damage) = 0;
};
