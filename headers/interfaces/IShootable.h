#pragma once
#include <boost/uuid/uuid.hpp>

class IShootable
{
protected:
	virtual ~IShootable() = default;

public:
	virtual boost::uuids::uuid Shot(boost::uuids::uuid uuid = {}) = 0;
};
