#pragma once

#include "utils/Uuid.h"
#include <memory>
#include <vector>

class BaseObj;

class IShootable
{
protected:
	virtual ~IShootable() = default;

public:
	//NOTE: every shell the tier fires at once, named by us - the world says where the volley has to end
	[[nodiscard]] virtual std::vector<Uuid> Volley(const std::vector<std::shared_ptr<BaseObj>>& objects) = 0;
	//NOTE: one shell of the host's volley, as the host named it - its next move puts it in its place
	virtual void Mirror(Uuid uuid) = 0;
};
