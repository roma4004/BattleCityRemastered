#pragma once

#include "entities/ObjRectangle.h"
#include <string>
#include <boost/uuid/uuid.hpp> //TODO: use PCH

struct BaseObjProperty
{
	using buuid = boost::uuids::uuid;

	ObjRectangle rect{};
	unsigned int color{0};
	int health{0};
	buuid uuid{};
	std::string name{};
	std::string fraction{};
	bool isAlive{true};
};
