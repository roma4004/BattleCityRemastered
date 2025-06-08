#pragma once
#include <boost/uuid/uuid.hpp>

class IShootable
{
	using buuid = boost::uuids::uuid;

protected:
	virtual ~IShootable() = default;

public:
	virtual buuid Shot(buuid uuid = {}) = 0;
};
