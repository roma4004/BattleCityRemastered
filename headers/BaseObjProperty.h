#pragma once

#include "ObjRectangle.h"

#include <string>
#include <boost/uuid/uuid.hpp>

struct BaseObjProperty
{
	ObjRectangle rect{};
	int color{0};
	int health{0};
	bool isAlive{true};
	boost::uuids::uuid uuid{};
	std::string name{};
	std::string fraction{};
};
